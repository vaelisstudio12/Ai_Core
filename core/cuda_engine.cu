// ==========================================
// 3. cuda_engine.cu (CUDA GPU Kernels)
// ==========================================
#include <cuda_runtime.h>
#include <math.h>

extern "C" {

// Bellek Yönetimi (Unified Memory destekli)
void* cuda_alloc_device(unsigned long bytes) {
    void* ptr;
    cudaMallocManaged(&ptr, bytes); // CPU ve GPU'nun ortak erişebildiği bellek
    return ptr;
}

void cuda_free_device(void* ptr) {
    cudaFree(ptr);
}

void cuda_copy_to_device(void* dst_gpu, const void* src_cpu, unsigned long bytes) {
    cudaMemcpy(dst_gpu, src_cpu, bytes, cudaMemcpyHostToDevice);
}

void cuda_copy_to_host(void* dst_cpu, const void* src_gpu, unsigned long bytes) {
    cudaMemcpy(dst_cpu, src_gpu, bytes, cudaMemcpyDeviceToHost);
}

// ----------------------------------------------------
// GPU: MATMUL (Matris Çarpımı - Tensor Core Uyumlu Bloklu)
// ----------------------------------------------------
#define TILE_SIZE 16

__global__ void kernel_matmul(const float* A, const float* B, float* C, int M, int K, int N) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    
    __shared__ float sA[TILE_SIZE][TILE_SIZE];
    __shared__ float sB[TILE_SIZE][TILE_SIZE];

    float sum = 0.0f;
    
    for (int t = 0; t < (K + TILE_SIZE - 1) / TILE_SIZE; ++t) {
        if (row < M && t * TILE_SIZE + threadIdx.x < K)
            sA[threadIdx.y][threadIdx.x] = A[row * K + t * TILE_SIZE + threadIdx.x];
        else
            sA[threadIdx.y][threadIdx.x] = 0.0f;

        if (t * TILE_SIZE + threadIdx.y < K && col < N)
            sB[threadIdx.y][threadIdx.x] = B[(t * TILE_SIZE + threadIdx.y) * N + col];
        else
            sB[threadIdx.y][threadIdx.x] = 0.0f;

        __syncthreads();

        for (int i = 0; i < TILE_SIZE; ++i) {
            sum += sA[threadIdx.y][i] * sB[i][threadIdx.x];
        }
        __syncthreads();
    }

    if (row < M && col < N) {
        C[row * N + col] = sum;
    }
}

void launch_cuda_matmul(const float* A, const float* B, float* C, int M, int K, int N) {
    dim3 threads(TILE_SIZE, TILE_SIZE);
    dim3 blocks((N + TILE_SIZE - 1) / TILE_SIZE, (M + TILE_SIZE - 1) / TILE_SIZE);
    kernel_matmul<<<blocks, threads>>>(A, B, C, M, K, N);
    cudaDeviceSynchronize();
}

// ----------------------------------------------------
// GPU: ACTIVATION FUNCTIONS
// ----------------------------------------------------
__global__ void kernel_relu(float* data, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        data[idx] = fmaxf(0.0f, data[idx]);
    }
}

void launch_cuda_relu(float* data, int size) {
    int threads = 256;
    int blocks = (size + threads - 1) / threads;
    kernel_relu<<<blocks, threads>>>(data, size);
    cudaDeviceSynchronize();
}

__global__ void kernel_sigmoid(float* data, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        data[idx] = 1.0f / (1.0f + expf(-data[idx]));
    }
}

void launch_cuda_sigmoid(float* data, int size) {
    int threads = 256;
    int blocks = (size + threads - 1) / threads;
    kernel_sigmoid<<<blocks, threads>>>(data, size);
    cudaDeviceSynchronize();
}

// ----------------------------------------------------
// GPU: DESCENT ALGORITHMS & LOSS
// ----------------------------------------------------
__global__ void kernel_sgd(float* weights, const float* grads, float lr, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        weights[idx] -= lr * grads[idx];
    }
}

void launch_cuda_sgd(float* weights, const float* grads, float lr, int size) {
    int threads = 256;
    int blocks = (size + threads - 1) / threads;
    kernel_sgd<<<blocks, threads>>>(weights, grads, lr, size);
    cudaDeviceSynchronize();
}

__global__ void kernel_quantile_loss_grad(const float* pred, const float* target, float* grad, float q, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        float error = target[idx] - pred[idx];
        grad[idx] = (error > 0) ? -q : (1.0f - q);
    }
}

void launch_cuda_quantile_loss_grad(const float* pred, const float* target, float* grad, float q, int size) {
    int threads = 256;
    int blocks = (size + threads - 1) / threads;
    kernel_quantile_loss_grad<<<blocks, threads>>>(pred, target, grad, q, size);
    cudaDeviceSynchronize();
}

} // extern "C"