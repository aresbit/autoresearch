#include "autoresearch.h"

typedef struct {
  u64 seen_tokens;
  u64 seen_bytes;
  u64 matched_tokens;
  u64 matched_bytes;
  u64 steps;
} ar_train_stats_t;

static u32 ar_arg_u32_or_default(int argc, char** argv, const c8* key, u32 fallback) {
  sp_for(i, (u32)argc) {
    if (sp_cstr_equal(argv[i], key) && (i + 1U) < (u32)argc) {
      return sp_parse_u32(sp_str_view(argv[i + 1]));
    }
  }
  return fallback;
}

static ar_train_stats_t ar_train_loop(const ar_vocab_t* vocab, sp_da(sp_str_t) tokens, u32 time_budget_s) {
  ar_train_stats_t stats = SP_ZERO_INITIALIZE();
  if (sp_dyn_array_size(tokens) == 0 || sp_dyn_array_size(vocab->entries) == 0) {
    return stats;
  }

  sp_tm_timer_t timer = sp_tm_start_timer();
  u64 budget_ns = sp_tm_s_to_ns(time_budget_s);
  u32 idx = 0;

  while (sp_tm_read_timer(&timer) < budget_ns) {
    sp_str_t token = tokens[idx];
    u32 hit = ar_vocab_lookup(vocab, token);

    stats.seen_tokens += 1;
    stats.seen_bytes += token.len;

    if (hit != SP_LIMIT_U32_MAX) {
      stats.matched_tokens += 1;
      stats.matched_bytes += token.len;
    }

    idx += 1;
    if (idx >= sp_dyn_array_size(tokens)) {
      idx = 0;
    }

    stats.steps += 1;
    if ((stats.steps % 4096U) == 0) {
      sp_os_sleep_ms(0.2);
    }
  }

  return stats;
}

int main(int argc, char** argv) {
  u32 time_budget_s = ar_arg_u32_or_default(argc, argv, "--time-budget", AR_TIME_BUDGET_SECONDS);

  ar_paths_t paths = ar_paths_new();
  ar_paths_ensure_layout(paths);

  ar_vocab_t vocab = ar_vocab_load(paths.vocab_path);
  if (sp_dyn_array_size(vocab.entries) == 0) {
    SP_LOG("missing vocab, run prepare first: {}", SP_FMT_STR(paths.vocab_path));
    return 1;
  }

  sp_str_t raw = ar_read_or_seed_corpus(paths);
  sp_str_t normalized = ar_normalize_whitespace(raw);
  sp_da(sp_str_t) tokens = ar_tokenize_words(normalized);

  sp_tm_timer_t total = sp_tm_start_timer();
  ar_train_stats_t stats = ar_train_loop(&vocab, tokens, time_budget_s);
  f64 training_seconds = sp_tm_ns_to_s_f(sp_tm_read_timer(&total));

  f64 token_hit_rate = stats.seen_tokens == 0 ? 0.0 : (f64)stats.matched_tokens / (f64)stats.seen_tokens;
  f64 byte_hit_rate = stats.seen_bytes == 0 ? 0.0 : (f64)stats.matched_bytes / (f64)stats.seen_bytes;
  f64 proxy_bpb = 2.0 - (token_hit_rate * 0.8 + byte_hit_rate * 1.2);
  if (proxy_bpb < 0.01) {
    proxy_bpb = 0.01;
  }

  u64 num_params = (u64)sp_dyn_array_size(vocab.entries) * 128ULL;
  f64 tokens_m = (f64)stats.seen_tokens / 1000000.0;

  SP_LOG("---");
  SP_LOG("val_bpb:          {}", SP_FMT_F64(proxy_bpb));
  SP_LOG("training_seconds: {}", SP_FMT_F64(training_seconds));
  SP_LOG("total_seconds:    {}", SP_FMT_F64(training_seconds));
  SP_LOG("peak_vram_mb:     {}", SP_FMT_F64(0.0));
  SP_LOG("mfu_percent:      {}", SP_FMT_F64(token_hit_rate * 100.0));
  SP_LOG("total_tokens_M:   {}", SP_FMT_F64(tokens_m));
  SP_LOG("num_steps:        {}", SP_FMT_U64(stats.steps));
  SP_LOG("num_params_M:     {}", SP_FMT_F64((f64)num_params / 1000000.0));
  SP_LOG("depth:            {}", SP_FMT_U32(1));

  ar_vocab_free(&vocab);
  return 0;
}
