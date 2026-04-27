# pffft

This directory contains fooyin's copy of pffft.

The upstream source files are kept in `src/`, along with the upstream
`README.md` and `LICENSE.txt`. fooyin builds only the float FFT code as the
static `fooyin_pffft` library and exposes it through the `PFFFT::PFFFT` CMake
alias.

When updating pffft, copy only the files needed by fooyin:

- `pffft.c`
- `pffft.h`
- `pffft_common.c`
- `pffft_priv_impl.h`
- `simd/pf_*.h`
