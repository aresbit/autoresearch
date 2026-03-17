# autoresearch

![teaser](progress.png)

A minimal autonomous-research playground, now rewritten in modern C.

The project no longer depends on Python/PyTorch. It uses a C toolchain + `Makefile`, and relies on `sp.h` as the primary runtime utility library.

## How it works

The repository is intentionally small. The main files are:

- `src/prepare.c`: one-time preparation phase (cache layout, corpus bootstrap, vocabulary build).
- `src/train.c`: training/evaluation loop entry (time-budgeted run + metrics summary).
- `src/ar_common.c`: shared tokenizer/corpus/vocab utilities.
- `include/sp.h`: single-header dependency (`spclib`) used by the codebase.
- `program.md`: autonomous-agent operating guide.

The training executable emits a summary block with the same key fields (`val_bpb`, `training_seconds`, `num_steps`, etc.) so runs remain easy to compare.

## Quick start

Requirements:

- C compiler (`cc`, `gcc`, or `clang`)
- `make`

Build and run:

```bash
make clean && make -j
make run-prepare
make run-train
```

Direct execution:

```bash
./bin/prepare --vocab-size 8192
./bin/train --time-budget 300
```

## Data layout

Runtime cache lives in:

- `~/.cache/autoresearch/data/corpus.txt`
- `~/.cache/autoresearch/tokenizer/vocab.tsv`

If `corpus.txt` does not exist, `prepare` writes a seed corpus automatically.

## Project structure

```text
include/
  autoresearch.h
  sp.h
src/
  sp_impl.c
  ar_common.c
  prepare.c
  train.c
Makefile
program.md
README.md
```

## Common commands

```bash
make               # build both binaries
make run-prepare   # build + run prepare phase
make run-train     # build + run train phase
make lint          # clang-tidy (if installed)
make format        # clang-format (if installed)
make clean
```

## License

MIT
