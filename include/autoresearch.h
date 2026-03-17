#ifndef AUTORESEARCH_H
#define AUTORESEARCH_H

#include "sp.h"

#define AR_MAX_SEQ_LEN 2048U
#define AR_TIME_BUDGET_SECONDS 300U
#define AR_DEFAULT_VOCAB_SIZE 8192U

typedef struct {
  sp_str_t token;
  u32 count;
  u32 bytes;
} ar_vocab_entry_t;

typedef struct {
  sp_da(ar_vocab_entry_t) entries;
  u64 total_tokens;
  u64 total_bytes;
} ar_vocab_t;

typedef struct {
  sp_str_t cache_dir;
  sp_str_t data_dir;
  sp_str_t tokenizer_dir;
  sp_str_t corpus_path;
  sp_str_t vocab_path;
} ar_paths_t;

ar_paths_t ar_paths_new(void);
void ar_paths_ensure_layout(ar_paths_t paths);
sp_str_t ar_read_or_seed_corpus(ar_paths_t paths);
sp_str_t ar_normalize_whitespace(sp_str_t raw);
sp_da(sp_str_t) ar_tokenize_words(sp_str_t normalized);

ar_vocab_t ar_build_vocab(sp_da(sp_str_t) tokens, u32 max_vocab);
void ar_vocab_sort_desc(ar_vocab_t* vocab);
bool ar_vocab_save(sp_str_t vocab_path, const ar_vocab_t* vocab);
ar_vocab_t ar_vocab_load(sp_str_t vocab_path);
u32 ar_vocab_lookup(const ar_vocab_t* vocab, sp_str_t token);
void ar_vocab_free(ar_vocab_t* vocab);

#endif
