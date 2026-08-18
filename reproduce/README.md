# Reproducer: fp16 mma SIGSEGV in PTX-mode `mma_impl`

Minimal trigger for the pointer-dereference bug at
`src/cuda-sim/instructions.cc:1952`: one warp, one m16n16k16
`wmma::mma_sync` on all-ones fp16 inputs (expected: C = 16.0 everywhere).

```bash
export CUDA_INSTALL_PATH=/usr/local/cuda   # CUDA 11.x tested
source ../setup_environment && make -C .. -j   # build the simulator
make                                           # build tc_bug (sm_70)
./tc_bug                                       # cwd holds gpgpusim.config
```

Config: stock `configs/tested-cfgs/SM7_QV100/gpgpusim.config`, unmodified.

- **Before the fix**: deterministic SIGSEGV at the first mma — fp16 1.0
  (bit pattern `0x3C00`) is reinterpreted as an address and dereferenced.
  See `gdb_backtrace.log` (note `print/x hex_val` → `$1 = 0x3c00`).
- **After the fix** (`&hex_val`): `tc_bug: C[0]=16.0 C[255]=16.0 (expect
  16.0) -> PASS`, exit 0.

nvcc host compiler must be gcc ≤ 10 for CUDA 11.1 (`-ccbin g++-10` in the
Makefile; adjust for your toolkit).
