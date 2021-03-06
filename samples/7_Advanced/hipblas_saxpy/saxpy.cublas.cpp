
#include <random>
#include <algorithm>
#include <iostream>
#include <cmath>

// header file for the GPU API
#include <cuda_runtime.h>
#include <cublas_v2.h>

#define N  (1024 * 500)

#define CHECK(cmd) \
{\
    cudaError_t error  = cmd;   \
    if (error != cudaSuccess) { \
      fprintf(stderr, "error: '%s'(%d) at %s:%d\n", cudaGetErrorString(error), error,__FILE__, __LINE__); \
      exit(EXIT_FAILURE);\
    }\
}

#define CHECK_BLAS(cmd)			\
{\
    cublasStatus_t error  = cmd;\
    if (error != CUBLAS_STATUS_SUCCESS) { \
      fprintf(stderr, "error: (%d) at %s:%d\n", error,__FILE__, __LINE__); \
      exit(EXIT_FAILURE);\
    }\
}

int main() {

  const float a = 100.0f;
  float x[N];
  float y[N], y_cpu_res[N], y_gpu_res[N];

  // initialize the input data
  std::default_random_engine random_gen;
  std::uniform_real_distribution<float> distribution(-N, N);
  std::generate_n(x, N, [&]() { return distribution(random_gen); });
  std::generate_n(y, N, [&]() { return distribution(random_gen); });
  std::copy_n(y, N, y_cpu_res);

  // Explicit GPU code:  

  size_t Nbytes = N*sizeof(float);
  float *x_gpu, *y_gpu;

  cublasHandle_t handle;

  cudaDeviceProp props;
  CHECK(cudaGetDeviceProperties(&props, 0/*deviceID*/));
  printf ("info: running on device %s\n", props.name);

  printf ("info: allocate host mem (%6.2f MB)\n", 2*Nbytes/1024.0/1024.0);
  printf ("info: allocate device mem (%6.2f MB)\n", 2*Nbytes/1024.0/1024.0);
  CHECK(cudaMalloc(&x_gpu, Nbytes));
  CHECK(cudaMalloc(&y_gpu, Nbytes));

  // Initialize the blas library
  CHECK_BLAS ( cublasCreate(&handle));

  // copy n elements from a vector in host memory space to a vector in GPU memory space
  printf ("info: copy Host2Device\n");
  CHECK_BLAS ( cublasSetVector(N, sizeof(*x), x, 1, x_gpu, 1));
  CHECK_BLAS ( cublasSetVector(N, sizeof(*y), y, 1, y_gpu, 1));

  printf ("info: launch 'saxpy' kernel\n");
  CHECK_BLAS ( cublasSaxpy(handle, N, &a, x_gpu, 1, y_gpu, 1));  

  cudaDeviceSynchronize();

  printf ("info: copy Device2Host\n");
  CHECK_BLAS ( cublasGetVector(N, sizeof(*y_gpu_res), y_gpu, 1, y_gpu_res, 1)); 

  // CPU implementation of saxpy
  for (int i = 0; i < N; i++) {
    y_cpu_res[i] = a * x[i] + y[i];
  }

  // verify the results
  int errors = 0;
  for (int i = 0; i < N; i++) {
    if (fabs(y_cpu_res[i] - y_gpu_res[i]) > fabs(y_cpu_res[i] * 0.0001f))
      errors++;
  }
  std::cout << errors << " errors" << std::endl;

  CHECK( cudaFree(x_gpu));
  CHECK( cudaFree(y_gpu));
  CHECK_BLAS( cublasDestroy(handle));

  return errors;
}
