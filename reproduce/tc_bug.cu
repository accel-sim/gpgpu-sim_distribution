// Yang Yang / Lars Christensen
// University of Virginia

// One warp, one m16n16k16 wmma::mma_sync with all-ones fp16 inputs.
// Expected result: C = A(1s) x B(1s) => every element 16.0.

// SIGSEGV at the first mma (tries to load from address 0x3C00 = fp16 1.0 bit pattern).

// Build: nvcc -ccbin g++-10 -arch=sm_70 -O2 tc_bug.cu -o tc_bug -lcudart

#include <cstdio>
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <mma.h>

#define N 16

__global__ void one_mma(const __half* A, const __half* B, float* C) {
  using namespace nvcuda;
  wmma::fragment<wmma::matrix_a, N, N, N, __half, wmma::row_major> a;
  wmma::fragment<wmma::matrix_b, N, N, N, __half, wmma::row_major> b;
  wmma::fragment<wmma::accumulator, N, N, N, float> c;
  wmma::fill_fragment(c, 0.f);
  wmma::load_matrix_sync(a, A, N);
  wmma::load_matrix_sync(b, B, N);
  wmma::mma_sync(c, a, b, c);            // <-- crashes here when unpatched
  wmma::store_matrix_sync(C, c, N, wmma::mem_row_major);
}

int main() {
  __half hA[N * N], hB[N * N];
  float hC[N * N];
  for (int i = 0; i < N * N; i++) { hA[i] = __float2half(1.0f); hB[i] = __float2half(1.0f); }
  __half *dA, *dB; float* dC;
  cudaMalloc(&dA, sizeof(hA)); cudaMalloc(&dB, sizeof(hB)); cudaMalloc(&dC, sizeof(hC));
  cudaMemcpy(dA, hA, sizeof(hA), cudaMemcpyHostToDevice);
  cudaMemcpy(dB, hB, sizeof(hB), cudaMemcpyHostToDevice);
  one_mma<<<1, 32>>>(dA, dB, dC);
  cudaMemcpy(hC, dC, sizeof(hC), cudaMemcpyDeviceToHost);
  int ok = 1;
  for (int i = 0; i < N * N; i++) ok &= (hC[i] == 16.0f);
  printf("tc_bug: C[0]=%.1f C[255]=%.1f (expect 16.0) -> %s\n",
         hC[0], hC[N * N - 1], ok ? "PASS" : "FAIL");
  return ok ? 0 : 1;
}
