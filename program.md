# autoresearch (C edition)

This is the operating protocol for autonomous experiments on the C implementation.

## Setup

To start a new run with the user:

1. Agree on a run tag (example: `mar18`).
2. Create a branch from `master`: `git checkout -b autoresearch/<tag>`.
3. Read in-scope files:
   - `README.md`
   - `src/prepare.c`
   - `src/train.c`
   - `src/ar_common.c`
4. Build once: `make clean && make -j`.
5. Ensure cache artifacts exist:
   - Run `./bin/prepare` if `~/.cache/autoresearch/tokenizer/vocab.tsv` is missing.
6. Initialize `results.tsv` (header only):
   - `commit\tval_bpb\tmemory_gb\tstatus\tdescription`

After setup is confirmed, begin experimentation.

## Experimentation rules

Each experiment is time-budgeted (default 300 seconds):

```bash
./bin/train --time-budget 300 > run.log 2>&1
```

### What you can modify

- `src/train.c` (primary experimental surface)
- `src/ar_common.c` (shared logic, tokenizer/vocab behavior) if needed
- `Makefile` (build flags / toolchain behavior)

### What you should keep stable

- Output summary keys and format (`val_bpb`, `training_seconds`, etc.)
- Cache paths under `~/.cache/autoresearch/`

## Output format

A successful run prints:

```text
---
val_bpb:          <float>
training_seconds: <float>
total_seconds:    <float>
peak_vram_mb:     <float>
mfu_percent:      <float>
total_tokens_M:   <float>
num_steps:        <int>
num_params_M:     <float>
depth:            <int>
```

Extract key metrics:

```bash
grep "^val_bpb:\|^peak_vram_mb:" run.log
```

## Logging results

Append each run to `results.tsv` (tab-separated):

```text
commit	val_bpb	memory_gb	status	description
```

- `status` is one of: `keep`, `discard`, `crash`
- For crashes: use `val_bpb=0.000000`, `memory_gb=0.0`

## Autonomous loop

Repeat forever until interrupted:

1. Check current git commit/branch.
2. Make one experimental change.
3. Commit.
4. Run: `./bin/train --time-budget 300 > run.log 2>&1`.
5. Parse metrics with `grep`.
6. If crash: inspect `tail -n 50 run.log`, fix or discard.
7. Append `results.tsv` (do not commit this file).
8. Keep commit only if metric improves; otherwise revert to previous good commit.

Never pause the loop to ask whether to continue unless explicitly instructed.
