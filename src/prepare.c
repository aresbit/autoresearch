#include "autoresearch.h"

static u32 ar_arg_u32_or_default(int argc, char** argv, const c8* key, u32 fallback) {
  sp_for(i, (u32)argc) {
    if (sp_cstr_equal(argv[i], key) && (i + 1U) < (u32)argc) {
      return sp_parse_u32(sp_str_view(argv[i + 1]));
    }
  }
  return fallback;
}

int main(int argc, char** argv) {
  u32 vocab_size = ar_arg_u32_or_default(argc, argv, "--vocab-size", AR_DEFAULT_VOCAB_SIZE);

  ar_paths_t paths = ar_paths_new();
  ar_paths_ensure_layout(paths);

  SP_LOG("cache_dir: {}", SP_FMT_STR(paths.cache_dir));
  SP_LOG("data_dir: {}", SP_FMT_STR(paths.data_dir));
  SP_LOG("tokenizer_dir: {}", SP_FMT_STR(paths.tokenizer_dir));

  sp_str_t raw = ar_read_or_seed_corpus(paths);
  sp_str_t normalized = ar_normalize_whitespace(raw);
  sp_da(sp_str_t) tokens = ar_tokenize_words(normalized);

  ar_vocab_t vocab = ar_build_vocab(tokens, vocab_size);
  bool ok = ar_vocab_save(paths.vocab_path, &vocab);

  SP_LOG("documents: {}", SP_FMT_U32(1));
  SP_LOG("tokens_total: {}", SP_FMT_U64(vocab.total_tokens));
  SP_LOG("bytes_total: {}", SP_FMT_U64(vocab.total_bytes));
  SP_LOG("vocab_size: {}", SP_FMT_U32(sp_dyn_array_size(vocab.entries)));
  SP_LOG("vocab_file: {}", SP_FMT_STR(paths.vocab_path));
  SP_LOG("status: {}", SP_FMT_CSTR(ok ? "ready" : "write_failed"));

  ar_vocab_free(&vocab);
  return ok ? 0 : 1;
}
