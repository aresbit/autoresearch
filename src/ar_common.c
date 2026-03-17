#include "autoresearch.h"

static bool ar_is_space(c8 c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v';
}

static sp_str_t ar_path_join2(sp_str_t a, sp_str_t b) {
  return sp_fs_join_path(a, b);
}

ar_paths_t ar_paths_new(void) {
  sp_str_t home = sp_os_get_env_var(sp_str_lit("HOME"));
  if (sp_str_empty(home)) {
    home = sp_str_lit(".");
  }

  sp_str_t cache_parent = ar_path_join2(home, sp_str_lit(".cache"));
  sp_str_t cache_dir = ar_path_join2(cache_parent, sp_str_lit("autoresearch"));
  sp_str_t data_dir = ar_path_join2(cache_dir, sp_str_lit("data"));
  sp_str_t tokenizer_dir = ar_path_join2(cache_dir, sp_str_lit("tokenizer"));

  sp_str_t corpus_path = ar_path_join2(data_dir, sp_str_lit("corpus.txt"));
  sp_str_t vocab_path = ar_path_join2(tokenizer_dir, sp_str_lit("vocab.tsv"));

  ar_paths_t paths = {
    .cache_dir = cache_dir,
    .data_dir = data_dir,
    .tokenizer_dir = tokenizer_dir,
    .corpus_path = corpus_path,
    .vocab_path = vocab_path,
  };
  return paths;
}

void ar_paths_ensure_layout(ar_paths_t paths) {
  if (!sp_fs_exists(paths.cache_dir)) {
    sp_fs_create_dir(paths.cache_dir);
  }
  if (!sp_fs_exists(paths.data_dir)) {
    sp_fs_create_dir(paths.data_dir);
  }
  if (!sp_fs_exists(paths.tokenizer_dir)) {
    sp_fs_create_dir(paths.tokenizer_dir);
  }
}

sp_str_t ar_read_or_seed_corpus(ar_paths_t paths) {
  if (!sp_fs_exists(paths.corpus_path)) {
    sp_io_writer_t writer = sp_io_writer_from_file(paths.corpus_path, SP_IO_WRITE_MODE_OVERWRITE);
    sp_io_write_cstr(&writer,
      "Autoresearch in C with sp.h. "
      "This corpus is a seed file. "
      "Put your own text into ~/.cache/autoresearch/data/corpus.txt for real training.\n"
      "Language models learn token statistics. "
      "Tokenizer quality affects bits per byte.\n");
    sp_io_writer_close(&writer);
  }
  return sp_io_read_file(paths.corpus_path);
}

sp_str_t ar_normalize_whitespace(sp_str_t raw) {
  c8* buffer = sp_alloc_n(c8, raw.len + 1);
  sp_str_t out = {
    .data = buffer,
    .len = 0,
  };
  u32 j = 0;
  bool prev_space = true;

  sp_for(i, raw.len) {
    c8 c = raw.data[i];
    bool is_space = ar_is_space(c);
    if (is_space) {
      if (!prev_space) {
        buffer[j++] = ' ';
      }
      prev_space = true;
    } else {
      buffer[j++] = c;
      prev_space = false;
    }
  }

  out.len = j;
  return sp_str_trim(out);
}

sp_da(sp_str_t) ar_tokenize_words(sp_str_t normalized) {
  sp_da(sp_str_t) split = sp_str_split_c8(normalized, ' ');
  sp_da(sp_str_t) tokens = SP_NULLPTR;
  sp_dyn_array_for(split, i) {
    sp_str_t t = sp_str_trim(split[i]);
    if (!sp_str_empty(t)) {
      sp_dyn_array_push(tokens, t);
    }
  }
  return tokens;
}

u32 ar_vocab_lookup(const ar_vocab_t* vocab, sp_str_t token) {
  sp_dyn_array_for(vocab->entries, i) {
    if (sp_str_equal(vocab->entries[i].token, token)) {
      return i;
    }
  }
  return SP_LIMIT_U32_MAX;
}

ar_vocab_t ar_build_vocab(sp_da(sp_str_t) tokens, u32 max_vocab) {
  ar_vocab_t vocab = SP_ZERO_INITIALIZE();

  sp_dyn_array_for(tokens, i) {
    sp_str_t token = tokens[i];
    u32 idx = ar_vocab_lookup(&vocab, token);
    if (idx == SP_LIMIT_U32_MAX) {
      ar_vocab_entry_t item = {
        .token = sp_str_copy(token),
        .count = 1,
        .bytes = token.len,
      };
      sp_dyn_array_push(vocab.entries, item);
    } else {
      vocab.entries[idx].count += 1;
    }
    vocab.total_tokens += 1;
    vocab.total_bytes += token.len;
  }

  ar_vocab_sort_desc(&vocab);
  while (sp_dyn_array_size(vocab.entries) > max_vocab) {
    u32 last = sp_dyn_array_size(vocab.entries) - 1;
    sp_free((void*)vocab.entries[last].token.data);
    sp_dyn_array_pop(vocab.entries);
  }

  return vocab;
}

void ar_vocab_sort_desc(ar_vocab_t* vocab) {
  u32 n = sp_dyn_array_size(vocab->entries);
  if (n < 2) {
    return;
  }

  sp_for(i, n) {
    u32 max_idx = i;
    sp_for_range(j, i + 1, n) {
      if (vocab->entries[j].count > vocab->entries[max_idx].count) {
        max_idx = j;
      }
    }
    if (max_idx != i) {
      SP_SWAP(ar_vocab_entry_t, vocab->entries[i], vocab->entries[max_idx]);
    }
  }
}

bool ar_vocab_save(sp_str_t vocab_path, const ar_vocab_t* vocab) {
  sp_io_writer_t writer = sp_io_writer_from_file(vocab_path, SP_IO_WRITE_MODE_OVERWRITE);
  sp_io_write_cstr(&writer, "token\tcount\tbytes\n");
  sp_dyn_array_for(vocab->entries, i) {
    sp_str_t line = sp_format("{}\t{}\t{}\n",
      SP_FMT_STR(vocab->entries[i].token),
      SP_FMT_U32(vocab->entries[i].count),
      SP_FMT_U32(vocab->entries[i].bytes));
    sp_io_write_str(&writer, line);
    sp_free((void*)line.data);
  }

  sp_io_writer_close(&writer);
  return true;
}

ar_vocab_t ar_vocab_load(sp_str_t vocab_path) {
  ar_vocab_t vocab = SP_ZERO_INITIALIZE();
  if (!sp_fs_exists(vocab_path)) {
    return vocab;
  }

  sp_str_t file = sp_io_read_file(vocab_path);
  sp_da(sp_str_t) lines = sp_str_split_c8(file, '\n');

  sp_dyn_array_for(lines, i) {
    sp_str_t line = sp_str_trim(lines[i]);
    if (sp_str_empty(line) || sp_str_starts_with(line, sp_str_lit("token\t"))) {
      continue;
    }

    sp_da(sp_str_t) cols = sp_str_split_c8(line, '\t');
    if (sp_dyn_array_size(cols) < 3) {
      continue;
    }

    ar_vocab_entry_t item = {
      .token = sp_str_copy(cols[0]),
      .count = sp_parse_u32(cols[1]),
      .bytes = sp_parse_u32(cols[2]),
    };
    sp_dyn_array_push(vocab.entries, item);
    vocab.total_tokens += item.count;
    vocab.total_bytes += (u64)item.count * (u64)item.bytes;
  }

  return vocab;
}

void ar_vocab_free(ar_vocab_t* vocab) {
  sp_dyn_array_for(vocab->entries, i) {
    sp_free((void*)vocab->entries[i].token.data);
  }
  sp_dyn_array_free(vocab->entries);
  vocab->entries = SP_NULLPTR;
  vocab->total_tokens = 0;
  vocab->total_bytes = 0;
}
