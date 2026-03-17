# autoresearch program: Vehicle Noise Suppression (RNN Filter Replacement)

This guide adapts the autoresearch loop to a small RNN-based denoiser for in-vehicle audio.

## Mission

Replace classical filtering with a small RNN model that improves speech quality under vehicle noise while meeting real-time constraints.

Primary objective:

- maximize quality (`PESQ`, `STOI`, `SNRi`)

Hard constraints (must pass):

- `latency_ms <= 10`
- `cpu_percent <= 60`
- no instability artifacts (NaN, clipping bursts, divergence)

## Repository contract

Use this layout in your project:

- `prepare.(c|py)`: data prep, split, feature extraction, fixed eval dataset index. (read-only during loop)
- `train.(c|py)`: train + evaluate + print final summary block. (main editable surface)
- `eval.(c|py)`: metric implementation for PESQ/STOI/SNRi/latency/cpu. (read-only during loop)
- `program_vehicle_noise.md`: this protocol.

## Setup checklist

1. Create branch: `git checkout -b autoresearch/vehicle-<tag>`.
2. Build once and run baseline once.
3. Verify fixed train/val/test splits exist.
4. Create `results.tsv` with header:

```text
commit	snri	pesq	stoi	latency_ms	cpu_percent	status	description
```

5. Confirm baseline metrics are logged.

## Standard run command

Always run a bounded experiment:

```bash
./bin/train --time-budget 300 > run.log 2>&1
```

(If Python project: `python train.py --time-budget 300 > run.log 2>&1`)

## Required output block

Your `train` command must print:

```text
---
snri:          <float>
pesq:          <float>
stoi:          <float>
latency_ms:    <float>
cpu_percent:   <float>
train_seconds: <float>
num_params_k:  <float>
```

## Keep/discard policy

For each run:

1. Parse summary from `run.log`.
2. If summary missing or runtime error: `status=crash`.
3. If any hard constraint violated: `status=discard`.
4. If constraints pass, compare to current best:
   - prefer higher `pesq`
   - tie-break with higher `stoi`
   - then higher `snri`
   - then lower `latency_ms`
5. Better -> keep commit. Otherwise -> revert commit.

## Experiment loop (autonomous)

Repeat until interrupted:

1. Inspect current best commit and metrics.
2. Make exactly one targeted change in `train` (or model file).
3. Commit with short message.
4. Run bounded experiment.
5. Extract metrics and append one TSV row.
6. Keep/discard per policy.

## Suggested search space (RNN denoiser)

Change only one item per experiment:

- RNN type: `GRU` vs `LSTM`
- hidden size: `{32, 64, 96, 128}`
- layers: `{1, 2, 3}`
- frame length / hop
- loss blend: `L1 + spectral + SI-SDR` weights
- feature set: log-mel vs STFT magnitude + phase features
- causal vs non-causal (causal for deployment)
- quantization-aware or int8 export path

## Safety rails

Immediately discard if:

- NaN loss or exploding gradients
- audible clipping bursts
- latency regression > 20% vs current best
- CPU usage beyond target budget

## Notes for student workflow

- Keep diffs tiny and readable.
- Run many fast experiments instead of a few giant changes.
- Trust fixed eval harness; do not tweak metrics to "win".
- Review top 5 kept runs weekly and merge ideas.
