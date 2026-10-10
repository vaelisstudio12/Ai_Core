// ==========================================
// 2. cuda_bridge.hpp (C/C++ Interface for GPU)
// ==========================================
#pragma once

extern "C" {
    // Memory Management
    void* cuda_alloc_device(unsigned long bytes);
    void cuda_free_device(void* ptr);
    void cuda_copy_to_device(void* dst_gpu, const void* src_cpu, unsigned long bytes);
    void cuda_copy_to_host(void* dst_cpu, const void* src_gpu, unsigned long bytes);

    // AI Kernels
    void launch_cuda_matmul(const float* A, const float* B, float* C, int M, int K, int N);
    void launch_cuda_relu(float* data, int size);
    void launch_cuda_sigmoid(float* data, int size);
    
    // Descent / Optimizer Kernels
    void launch_cuda_sgd(float* weights, const float* grads, float lr, int size);
    void launch_cuda_quantile_loss_grad(const float* pred, const float* target, float* grad, float q, int size);
}