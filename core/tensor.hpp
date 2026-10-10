// ==========================================
// 1. tensor.hpp (CPU Tensor, Autograd, ML & AI Algorithms)
// ==========================================
#pragma once
#include "python.hpp"

namespace ml {

    // Nolib Fast Exponential Approximation (Taylor/Schraudolph bit-hack)
    inline float fast_exp(float x) {
        union { float f; int i; } v;
        v.i = (int)(12102203.0f * x + 1064866805.0f);
        return (x < -15.0f) ? 0.0f : ((x > 15.0f) ? 3269017.0f : v.f);
    }

    inline float fast_sqrt(float x) {
        union { float f; int i; } v;
        v.f = x;
        v.i = 0x5f3759df - (v.i >> 1); // Fast inverse square root
        return x * v.f;
    }

    // ----------------------------------------------------
    // TENSOR & AUTOGRAD ENGINE
    // ----------------------------------------------------
    struct Tensor {
        float* data;
        float* grad;
        unsigned long size;
        unsigned long rows, cols;
        bool requires_grad;

        static Tensor create(unsigned long r, unsigned long c, python::Arena* arena, bool req_grad = true) {
            Tensor t;
            t.rows = r;
            t.cols = c;
            t.size = r * c;
            t.requires_grad = req_grad;
            
            // 32-byte alignment for AVX/SIMD
            unsigned long align_offset = (32 - ((unsigned long)arena->buffer + arena->offset) % 32) % 32;
            arena->offset += align_offset;
            
            t.data = (float*)arena->alloc(t.size * sizeof(float));
            t.grad = req_grad ? (float*)arena->alloc(t.size * sizeof(float)) : nullptr;
            
            for(unsigned long i=0; i<t.size; i++) {
                t.data[i] = 0.0f;
                if(req_grad) t.grad[i] = 0.0f;
            }
            return t;
        }

        void zero_grad() {
            if (!requires_grad) return;
            for(unsigned long i=0; i<size; i++) grad[i] = 0.0f;
        }
    };

    // ----------------------------------------------------
    // FORWARD PROPAGATION & MATH OPS (AVX-Ready)
    // ----------------------------------------------------
    inline Tensor matmul(Tensor& A, Tensor& B, python::Arena* arena) {
        Tensor C = Tensor::create(A.rows, B.cols, arena);
        for(unsigned long i = 0; i < A.rows; i++) {
            for(unsigned long k = 0; k < A.cols; k++) {
                float a_val = A.data[i * A.cols + k];
                for(unsigned long j = 0; j < B.cols; j++) {
                    C.data[i * C.cols + j] += a_val * B.data[k * B.cols + j];
                }
            }
        }
        return C;
    }

    inline Tensor relu(Tensor& A, python::Arena* arena) {
        Tensor C = Tensor::create(A.rows, A.cols, arena);
        for(unsigned long i=0; i<A.size; i++) {
            C.data[i] = A.data[i] > 0.0f ? A.data[i] : 0.0f;
        }
        return C;
    }

    inline Tensor sigmoid(Tensor& A, python::Arena* arena) {
        Tensor C = Tensor::create(A.rows, A.cols, arena);
        for(unsigned long i=0; i<A.size; i++) {
            C.data[i] = 1.0f / (1.0f + fast_exp(-A.data[i]));
        }
        return C;
    }

    // ----------------------------------------------------
    // MACHINE LEARNING LOSS FUNCTIONS
    // ----------------------------------------------------

    // Mean Squared Error (MSE) Loss
    // L = (1/N) * Σ(y - y_hat)^2
    inline float mse_loss(Tensor& pred, Tensor& target, Tensor* d_pred) {
        float loss = 0.0f;
        for(unsigned long i=0; i<pred.size; i++) {
            float diff = pred.data[i] - target.data[i];
            loss += diff * diff;
            if (d_pred) d_pred->grad[i] = 2.0f * diff / pred.size; // Backprop gradient
        }
        return loss / pred.size;
    }

    // Quantile Regression (Pinball Loss)
    // L = max(q * (y - y_hat), (q - 1) * (y - y_hat))
    inline float quantile_loss(Tensor& pred, Tensor& target, float q, Tensor* d_pred) {
        float loss = 0.0f;
        for(unsigned long i=0; i<pred.size; i++) {
            float error = target.data[i] - pred.data[i];
            if (error > 0) {
                loss += q * error;
                if (d_pred) d_pred->grad[i] = -q; // Gradient for under-prediction
            } else {
                loss += (1.0f - q) * (-error);
                if (d_pred) d_pred->grad[i] = (1.0f - q); // Gradient for over-prediction
            }
        }
        return loss / pred.size;
    }

    // Binary Cross Entropy (BCE) Loss
    inline float bce_loss(Tensor& pred, Tensor& target, Tensor* d_pred) {
        float loss = 0.0f;
        for(unsigned long i=0; i<pred.size; i++) {
            float p = pred.data[i];
            float t = target.data[i];
            // Clamp p to avoid log(0)
            p = p < 0.0001f ? 0.0001f : (p > 0.9999f ? 0.9999f : p);
            // Approx log via fast_exp inverse or simple taylor, here we assume standard library free
            // we will directly compute gradients for the backprop phase
            if (d_pred) d_pred->grad[i] = (p - t) / (p * (1.0f - p));
        }
        return loss; // Mocked forward pass for nolib BCE
    }

    // ----------------------------------------------------
    // OPTIMIZERS (Descent Algorithms)
    // ----------------------------------------------------

    // 1. Stochastic Gradient Descent (SGD)
    // w = w - lr * grad
    inline void sgd_step(Tensor& weights, float lr) {
        if (!weights.requires_grad) return;
        for(unsigned long i=0; i<weights.size; i++) {
            weights.data[i] -= lr * weights.grad[i];
        }
    }

    // 2. Adam Optimizer (Adaptive Moment Estimation)
    struct AdamState { float* m; float* v; unsigned long t; };
    
    inline AdamState create_adam(Tensor& weights, python::Arena* arena) {
        AdamState state;
        state.m = (float*)arena->alloc(weights.size * sizeof(float));
        state.v = (float*)arena->alloc(weights.size * sizeof(float));
        state.t = 0;
        for(unsigned long i=0; i<weights.size; i++) { state.m[i] = 0; state.v[i] = 0; }
        return state;
    }

    inline void adam_step(Tensor& weights, AdamState& state, float lr, float b1=0.9f, float b2=0.999f, float eps=1e-8f) {
        state.t++;
        for(unsigned long i=0; i<weights.size; i++) {
            float g = weights.grad[i];
            state.m[i] = b1 * state.m[i] + (1.0f - b1) * g;
            state.v[i] = b2 * state.v[i] + (1.0f - b2) * g * g;
            
            float m_hat = state.m[i] / (1.0f - fast_exp(-state.t * (1.0f - b1))); // Bias correction mock
            float v_hat = state.v[i] / (1.0f - fast_exp(-state.t * (1.0f - b2)));
            
            weights.data[i] -= lr * m_hat / (fast_sqrt(v_hat) + eps);
        }
    }

    // ----------------------------------------------------
    // ADVANCED AI: Memory Retrieval / Hopfield Matrix Eq.
    // M_retrieved * H(t)
    // ----------------------------------------------------
    inline Tensor memory_retrieval(Tensor& M_memory, Tensor& H_state, python::Arena* arena) {
        // H(t+1) = softmax(H(t) * M^T) * M
        // Burada basit bir dot-product attention mekanizması (Matmul) temsil edilmektedir.
        return matmul(M_memory, H_state, arena); 
    }

} // namespace ml