/*
 * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* OpenFlow packet encoder/decoder generator. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "generator.h"
#include "openflow.h"

/* Top expression. */
struct exp *top = NULL;

/* Expression allocation. */
static struct exp *
exp_new(void) {
  struct exp *exp;

  exp = calloc(sizeof(struct exp), 1);
  if (exp == NULL) {
    return NULL;
  }

  return exp;
}

/* Expression free. */
static void
exp_free(struct exp *exp) {
  free(exp);
}

#if 0
/* Expression list allocation. */
static struct exp_list *
exp_list_new(void) {
  struct exp_list *elist;

  elist = calloc(sizeof(struct exp_list), 1);
  if (elist == NULL) {
    return NULL;
  }

  TAILQ_INIT(&elist->tailq);

  return elist;
}
#endif

#if 0
/* Print expression. */
static void
exp_print(struct exp *exp) {
  switch (exp->type) {
    case EXP_TYPE_DECLARATION:
      printf("Exp Print: %s %s\n", exp->decl.type, exp->decl.name);
      break;
    case EXP_TYPE_ARRAY_DECLARATION:
      printf("Exp Print: %s %s[%d]\n", exp->decl.type, exp->decl.name,
             exp->decl.array_size);
      break;
    case EXP_TYPE_STRUCT_DECLARATION:
      printf("Exp Print: struct %s %s\n", exp->decl.type, exp->decl.name);
      break;
    case EXP_TYPE_ARRAY_STRUCT_DECLARATION:
      printf("Exp Print: struct %s %s[%d]\n", exp->decl.type, exp->decl.name,
             exp->decl.array_size);
      break;
    case EXP_TYPE_ENUM_DECLARATION:
      printf("Exp Print: enum %s\n", exp->decl.name);
      break;
    case EXP_TYPE_LIST: {
      struct exp *e;
      printf("Struct %s\n", exp->elist.name);
      TAILQ_FOREACH(e, &exp->elist.tailq, entry) {
        exp_print(e);
      }
    }
    break;
    case EXP_TYPE_ENUM: {
      struct exp *e;
      printf("Enum %s\n", exp->elist.name);
      TAILQ_FOREACH(e, &exp->elist.tailq, entry) {
        exp_print(e);
      }
    }
    break;
  }
}
#endif

/* Identify type. */
static enum decl_type
decl_type_lookup(char *type_str) {
  if (strcmp(type_str, "char") == 0) {
    return DECL_TYPE_CHAR;
  } else if (strcmp(type_str, "uint8_t") == 0) {
    return DECL_TYPE_UCHAR;
  } else if (strcmp(type_str, "uint16_t") == 0) {
    return DECL_TYPE_WORD;
  } else if (strcmp(type_str, "uint32_t") == 0) {
    return DECL_TYPE_LONG;
  } else if (strcmp(type_str, "uint64_t") == 0) {
    return DECL_TYPE_LLONG;
  } else {
    return DECL_TYPE_UNKNOWN;
  }
}

static void
output_macros(void) {
  fprintf(fp_header, "\n");
  fprintf(fp_header, "#define TRACE_STR_ADD(n, ...) {             \\\n");
  fprintf(fp_header, "    n = snprintf(__VA_ARGS__);              \\\n");
  fprintf(fp_header, "    if (n >= 0) {                           \\\n");
  fprintf(fp_header, "      if (max_len > (size_t) n) {           \\\n");
  fprintf(fp_header, "        max_len -= (size_t) n;              \\\n");
  fprintf(fp_header, "        str += n;                           \\\n");
  fprintf(fp_header, "      } else {                              \\\n");
  fprintf(fp_header, "        return LAGOPUS_RESULT_OUT_OF_RANGE; \\\n");
  fprintf(fp_header, "      }                                     \\\n");
  fprintf(fp_header, "    } else {                                \\\n");
  fprintf(fp_header, "      return LAGOPUS_RESULT_ANY_FAILURES;   \\\n");
  fprintf(fp_header, "    }                                       \\\n");
  fprintf(fp_header, "  }\n");
  fprintf(fp_header, "\n");
  fprintf(fp_header, "#define ARRAY_NOT_EMPTY(A) (A != 0)\n");
}

void
output_top(const char *hname) {
  char buf[256];
  size_t len;
  int i;

  char licence[] =
    "/*\n"
    " * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.\n"
    " *\n"
    " * Licensed under the Apache License, Version 2.0 (the \"License\");\n"
    " * you may not use this file except in compliance with the License.\n"
    " * You may obtain a copy of the License at\n"
    " *\n"
    " *   http://www.apache.org/licenses/LICENSE-2.0\n"
    " *\n"
    " * Unless required by applicable law or agreed to in writing, software\n"
    " * distributed under the License is distributed on an \"AS IS\" BASIS,\n"
    " * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
    " * See the License for the specific language governing permissions and\n"
    " * limitations under the License.\n"
    " */\n";

  len = strlen(hname);
  if (len >= sizeof(buf)) {
    len = sizeof(buf) - 1;
  }
  strncpy(buf, hname, len);
  buf[len] = '\0';

  for (i = 0; i < (int)len; i++) {
    if (isalpha(buf[i])) {
      buf[i] = (char)toupper(buf[i]);
    } else if (! isdigit(buf[i])) {
      buf[i] = '_';
    }
  }
  fprintf(fp_header, "%s", licence);
  fprintf(fp_header, "\n");
  fprintf(fp_header, "/**\n");
  fprintf(fp_header, " * @file\t%s\n", hname);
  fprintf(fp_header, " */\n");
  fprintf(fp_header, "\n");
  fprintf(fp_header, "#ifndef __%s_INCLUDED__\n", buf);
  fprintf(fp_header, "#define __%s_INCLUDED__\n\n", buf);

  fprintf(fp_header, "#ifdef __cplusplus\n");
  fprintf(fp_header, "extern \"C\" {\n");
  fprintf(fp_header, "#endif\n\n");

  /* Header. */
  fprintf(fp_header, "#include <stdio.h>\n");
  fprintf(fp_header, "#include <stdint.h>\n");
  fprintf(fp_header, "#include <inttypes.h>\n");
  fprintf(fp_header, "#include <string.h>\n");
  fprintf(fp_header, "#include \"lagopus/pbuf.h\"\n");
  fprintf(fp_header, "#include \"lagopus_apis.h\"\n");
  fprintf(fp_header, "#include \"openflow.h\"\n");
  //fprintf(fp_header, "#include \"%s\"\n", hname);

  /* output macros. */
  output_macros();

  /* Source. */
  fprintf(fp_source, "#include \"%s\"\n", hname);
  fprintf(fp_source, "#include \"ofp_header_handler.h\"\n");
}

void
output_bottom(void) {
  fprintf(fp_header, "\n#ifdef __cplusplus\n");
  fprintf(fp_header, "}\n");
  fprintf(fp_header, "#endif\n\n");

  fprintf(fp_header, "#endif\n");
}

/* Generate encoder function. */
static void
exp_generate_encoder_function(struct exp_list *elist, FILE *fp) {
  fprintf(fp, "\n");
  fprintf(fp, "lagopus_result_t\n");
  fprintf(fp, "%s_encode(struct pbuf *pbuf, const struct %s *packet)",
          elist->name, elist->name);
}

/* Generate encoder function (list). */
static void
exp_generate_list_encoder_function(struct exp_list *elist, FILE *fp) {
  fprintf(fp, "\n");
  fprintf(fp, "lagopus_result_t\n");
  fprintf(fp,
          "%s_encode_list(struct pbuf_list *pbuf_list, struct pbuf **pbuf,\n",
          elist->name);
  fprintf(fp, "    const struct %s *packet)",
          elist->name);
}

/* Generate encoder. */
static void
exp_generate_encoder(struct exp_list *elist) {
  struct exp *exp;

  /* Header output. */
  exp_generate_encoder_function(elist, fp_header);
  fprintf(fp_header, ";\n");

  /* Source output. */
  exp_generate_encoder_function(elist, fp_source);
  fprintf(fp_source, "\n");
  fprintf(fp_source, "{\n");
  fprintf(fp_source, "  /* Size check. */\n");
  fprintf(fp_source, "  if (pbuf->plen < sizeof(struct %s))\n", elist->name);
  fprintf(fp_source, "    return LAGOPUS_RESULT_OUT_OF_RANGE;\n");
  fprintf(fp_source, "\n");

  fprintf(fp_source, "  /* Encode packet. */\n");
  TAILQ_FOREACH(exp, &elist->tailq, entry) {
    enum decl_type type;

    /* Look up declaration type. */
    type = decl_type_lookup(exp->decl.type);

    /* In case of simple declaration. */
    if (exp->type == EXP_TYPE_DECLARATION) {
      switch (type) {
        case DECL_TYPE_CHAR:
        case DECL_TYPE_UCHAR:
          fprintf(fp_source, "  ENCODE_PUTC");
          break;
        case DECL_TYPE_WORD:
          fprintf(fp_source, "  ENCODE_PUTW");
          break;
        case DECL_TYPE_LONG:
          fprintf(fp_source, "  ENCODE_PUTL");
          break;
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "  ENCODE_PUTLL");
          break;
        case DECL_TYPE_UNKNOWN:
          fprintf(stderr, "Error: unknown type %s\n", exp->decl.type);
          exit (1);
          break;
      }

      /* Declaration name. */
      fprintf(fp_source, "(packet->%s);\n", exp->decl.name);
    } else if (exp->type == EXP_TYPE_STRUCT_DECLARATION) {
      fprintf(fp_source, "  if (%s_encode(pbuf, &packet->%s) < 0) {\n",
              exp->decl.type, exp->decl.name);
      fprintf(fp_source, "    return LAGOPUS_RESULT_OUT_OF_RANGE;\n");
      fprintf(fp_source, "  }\n");
    } else if (exp->type == EXP_TYPE_ARRAY_STRUCT_DECLARATION) {

    } else {
      switch (type) {
        case DECL_TYPE_WORD:
        case DECL_TYPE_LONG:
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "  for (int i = 0; i < %d; i++) {\n",
                  exp->decl.array_size);
          break;
        default:
          break;
      }

      switch (type) {
        case DECL_TYPE_WORD:
          fprintf(fp_source, "    ENCODE_PUTW(packet->%s[i]);\n",
                  exp->decl.name);
          break;
        case DECL_TYPE_LONG:
          fprintf(fp_source, "    ENCODE_PUTL(packet->%s[i]);\n",
                  exp->decl.name);
          break;
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "    ENCODE_PUTLL(packet->%s[i]);\n",
                  exp->decl.name);
          break;
        default:
          fprintf(fp_source, "  ENCODE_PUT(packet->%s, %d);\n",
                  exp->decl.name, exp->decl.array_size);
          break;
      }

      switch (type) {
        case DECL_TYPE_WORD:
        case DECL_TYPE_LONG:
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "  }\n");
          break;
        default:
          break;
      }
    }
  }
  fprintf(fp_source, "\n");
  fprintf(fp_source, "  return LAGOPUS_RESULT_OK;\n");
  fprintf(fp_source, "}\n");
}

/* Generate list encoder. */
static void
exp_generate_list_encoder(struct exp_list *elist) {
  /* Header output. */
  exp_generate_list_encoder_function(elist, fp_header);
  fprintf(fp_header, ";\n");

  /* Source output. */
  exp_generate_list_encoder_function(elist, fp_source);
  fprintf(fp_source, "\n");
  fprintf(fp_source, "{\n");
  fprintf(fp_source, "  struct pbuf *before_pbuf;\n");
  fprintf(fp_source, "  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;\n");
  fprintf(fp_source, "\n");
  fprintf(fp_source, "  if (pbuf_list == NULL) {\n");
  fprintf(fp_source, "    return %s_encode(*pbuf, packet);\n", elist->name);
  fprintf(fp_source, "  }\n");
  fprintf(fp_source, "\n");
  fprintf(fp_source, "  *pbuf = pbuf_list_last_get(pbuf_list);\n");
  fprintf(fp_source, "  if (*pbuf == NULL) {\n");
  fprintf(fp_source, "    return LAGOPUS_RESULT_NO_MEMORY;\n");
  fprintf(fp_source, "  }\n");
  fprintf(fp_source, "\n");
  fprintf(fp_source, "  ret = %s_encode(*pbuf, packet);\n", elist->name);
  fprintf(fp_source, "  if (ret == LAGOPUS_RESULT_OUT_OF_RANGE) {\n");
  fprintf(fp_source, "    before_pbuf = *pbuf;\n");
  fprintf(fp_source, "    *pbuf = pbuf_alloc(OFP_PACKET_MAX_SIZE);\n");
  fprintf(fp_source, "    if (*pbuf == NULL) {\n");
  fprintf(fp_source, "      return LAGOPUS_RESULT_NO_MEMORY;\n");
  fprintf(fp_source, "    }\n");
  fprintf(fp_source, "    pbuf_list_add(pbuf_list, *pbuf);\n");
  fprintf(fp_source, "    (*pbuf)->plen = OFP_PACKET_MAX_SIZE;\n");
  fprintf(fp_source, "    ret = ofp_header_mp_copy(*pbuf, before_pbuf);\n");
  fprintf(fp_source, "    if (ret != LAGOPUS_RESULT_OK) {\n");
  fprintf(fp_source, "      return ret;\n");
  fprintf(fp_source, "    }\n");
  fprintf(fp_source, "    ret = %s_encode(*pbuf, packet);\n", elist->name);
  fprintf(fp_source, "  }\n");
  fprintf(fp_source, "\n");
  fprintf(fp_source, "  return ret;\n");
  fprintf(fp_source, "}\n");
}

/* Generate decoder function. */
static void
exp_generate_decoder_function(struct exp_list *elist, FILE *fp) {
  fprintf(fp, "\n");
  fprintf(fp, "lagopus_result_t\n");
  fprintf(fp, "%s_decode(struct pbuf *pbuf, struct %s *packet)",
          elist->name, elist->name);
}

/* Generate decoder. */
static void
exp_generate_decoder(struct exp_list *elist) {
  struct exp *exp;
  int ofp_header = 0;

  /* Header output. */
  exp_generate_decoder_function(elist, fp_header);
  fprintf(fp_header, ";\n");

  /* ofp_header check. */
#if 0
  if (strcmp(elist->name, "ofp_header") != 0) {
    TAILQ_FOREACH(exp, &elist->tailq, entry) {
      if (exp->type == EXP_TYPE_STRUCT_DECLARATION &&
          strcmp(exp->decl.type, "ofp_header") == 0) {
        ofp_header = 1;
      }
    }
  }
#endif

  /* Source output. */
  exp_generate_decoder_function(elist, fp_source);
  fprintf(fp_source, "\n");
  fprintf(fp_source, "{\n");
  fprintf(fp_source, "  /* Size check. */\n");
  if (ofp_header == 1) {
    fprintf(fp_source,
            "  if (pbuf->plen < (sizeof(struct %s) - sizeof(struct ofp_header)))\n",
            elist->name);
  } else {
    fprintf(fp_source, "  if (pbuf->plen < sizeof(struct %s))\n", elist->name);
  }
  fprintf(fp_source, "    return LAGOPUS_RESULT_OUT_OF_RANGE;\n");
  fprintf(fp_source, "\n");

  fprintf(fp_source, "  /* Decode packet. */\n");
  TAILQ_FOREACH(exp, &elist->tailq, entry) {
    enum decl_type type;

    /* Look up declaration type. */
    type = decl_type_lookup(exp->decl.type);

    /* In case of simple declaration. */
    if (exp->type == EXP_TYPE_DECLARATION) {
      switch (type) {
        case DECL_TYPE_CHAR:
        case DECL_TYPE_UCHAR:
          fprintf(fp_source, "  DECODE_GETC");
          break;
        case DECL_TYPE_WORD:
          fprintf(fp_source, "  DECODE_GETW");
          break;
        case DECL_TYPE_LONG:
          fprintf(fp_source, "  DECODE_GETL");
          break;
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "  DECODE_GETLL");
          break;
        case DECL_TYPE_UNKNOWN:
          fprintf(stderr, "Error: unknown type %s\n", exp->decl.type);
          exit (1);
          break;
      }

      /* Declaration name. */
      fprintf(fp_source, "(packet->%s);\n", exp->decl.name);
    } else if (exp->type == EXP_TYPE_STRUCT_DECLARATION) {
#if 1 /* Enable all header decode. */
      if (strcmp(exp->decl.type, "ofp_header") == 0) {
        fprintf(fp_source, "  if (%s_decode(pbuf, &packet->%s) < 0) {\n",
                exp->decl.type, exp->decl.name);
        fprintf(fp_source, "    return LAGOPUS_RESULT_OUT_OF_RANGE;\n");
        fprintf(fp_source, "  }\n");
      }
#endif
    } else if (exp->type == EXP_TYPE_ARRAY_STRUCT_DECLARATION) {

    } else {
      switch (type) {
        case DECL_TYPE_WORD:
        case DECL_TYPE_LONG:
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "  for (int i = 0; i < %d; i++) {\n",
                  exp->decl.array_size);
          break;
        default:
          break;
      }

      switch (type) {
        case DECL_TYPE_WORD:
          fprintf(fp_source, "    DECODE_GETW(packet->%s[i]);\n",
                  exp->decl.name);
          break;
        case DECL_TYPE_LONG:
          fprintf(fp_source, "    DECODE_GETL(packet->%s[i]);\n",
                  exp->decl.name);
          break;
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "    DECODE_GETLL(packet->%s[i]);\n",
                  exp->decl.name);
          break;
        default:
          fprintf(fp_source, "  DECODE_GET(packet->%s, %d);\n",
                  exp->decl.name, exp->decl.array_size);
          break;
      }

      switch (type) {
        case DECL_TYPE_WORD:
        case DECL_TYPE_LONG:
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "  }\n");
          break;
        default:
          break;
      }
    }
  }

  fprintf(fp_source, "\n");
  fprintf(fp_source, "  return LAGOPUS_RESULT_OK;\n");
  fprintf(fp_source, "}\n");
}

/* Generate sneak decoder function. */
static void
exp_generate_sneak_decoder_function(struct exp_list *elist, FILE *fp) {
  fprintf(fp, "\n");
  fprintf(fp, "lagopus_result_t\n");
  fprintf(fp, "%s_decode_sneak(struct pbuf *pbuf, struct %s *packet)",
          elist->name, elist->name);
}

/* Generate sneak decoder. */
static void
exp_generate_sneak_decoder(struct exp_list *elist) {
  struct exp *exp;
  int ofp_header = 0;

  /* Header output. */
  exp_generate_sneak_decoder_function(elist, fp_header);
  fprintf(fp_header, ";\n");

  /* ofp_header check. */
#if 0
  if (strcmp(elist->name, "ofp_header") != 0) {
    TAILQ_FOREACH(exp, &elist->tailq, entry) {
      if (exp->type == EXP_TYPE_STRUCT_DECLARATION &&
          strcmp(exp->decl.type, "ofp_header") == 0) {
        ofp_header = 1;
      }
    }
  }
#endif

  /* Source output. */
  exp_generate_sneak_decoder_function(elist, fp_source);
  fprintf(fp_source, "\n");
  fprintf(fp_source, "{\n");
  fprintf(fp_source, "  /* Size check. */\n");
  if (ofp_header == 1) {
    fprintf(fp_source,
            "  if (pbuf->plen < (sizeof(struct %s) - sizeof(struct ofp_header)))\n",
            elist->name);
  } else {
    fprintf(fp_source, "  if (pbuf->plen < sizeof(struct %s))\n", elist->name);
  }
  fprintf(fp_source, "    return LAGOPUS_RESULT_OUT_OF_RANGE;\n");
  fprintf(fp_source, "\n");

  fprintf(fp_source, "  /* Decode packet. */\n");
  TAILQ_FOREACH(exp, &elist->tailq, entry) {
    enum decl_type type;

    /* Look up declaration type. */
    type = decl_type_lookup(exp->decl.type);

    /* In case of simple declaration. */
    if (exp->type == EXP_TYPE_DECLARATION) {
      switch (type) {
        case DECL_TYPE_CHAR:
        case DECL_TYPE_UCHAR:
          fprintf(fp_source, "  DECODE_GETC");
          break;
        case DECL_TYPE_WORD:
          fprintf(fp_source, "  DECODE_GETW");
          break;
        case DECL_TYPE_LONG:
          fprintf(fp_source, "  DECODE_GETL");
          break;
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "  DECODE_GETLL");
          break;
        case DECL_TYPE_UNKNOWN:
          fprintf(stderr, "Error: unknown type %s\n", exp->decl.type);
          exit (1);
          break;
      }

      /* Declaration name. */
      fprintf(fp_source, "(packet->%s);\n", exp->decl.name);
    } else if (exp->type == EXP_TYPE_STRUCT_DECLARATION) {
#if 1 /* Enable all header decode. */
      if (strcmp(exp->decl.type, "ofp_header") == 0) {
        fprintf(fp_source, "  if (%s_decode(pbuf, &packet->%s) < 0) {\n",
                exp->decl.type, exp->decl.name);
        fprintf(fp_source, "    return LAGOPUS_RESULT_OUT_OF_RANGE;\n");
        fprintf(fp_source, "  }\n");
      }
#endif
    } else if (exp->type == EXP_TYPE_ARRAY_STRUCT_DECLARATION) {

    } else {
      switch (type) {
        case DECL_TYPE_WORD:
        case DECL_TYPE_LONG:
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "  for (int i = 0; i < %d; i++) {\n",
                  exp->decl.array_size);
          break;
        default:
          break;
      }

      switch (type) {
        case DECL_TYPE_WORD:
          fprintf(fp_source, "    DECODE_GETW(packet->%s[i]);\n",
                  exp->decl.name);
          break;
        case DECL_TYPE_LONG:
          fprintf(fp_source, "    DECODE_GETL(packet->%s[i]);\n",
                  exp->decl.name);
          break;
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "    DECODE_GETLL(packet->%s[i]);\n",
                  exp->decl.name);
          break;
        default:
          fprintf(fp_source, "  DECODE_GET(packet->%s, %d);\n",
                  exp->decl.name, exp->decl.array_size);
          break;
      }

      switch (type) {
        case DECL_TYPE_WORD:
        case DECL_TYPE_LONG:
        case DECL_TYPE_LLONG:
          fprintf(fp_source, "  }\n");
          break;
        default:
          break;
      }
    }
  }

  fprintf(fp_source, "  DECODE_REWIND(sizeof(struct %s));\n", elist->name);
  fprintf(fp_source, "\n");
  fprintf(fp_source, "  return LAGOPUS_RESULT_OK;\n");
  fprintf(fp_source, "}\n");
}

/* TODO : Delete debug generator. */
/* Generate decoder function. */
static void
exp_generate_debug_function(struct exp_list *elist, FILE *fp) {
  fprintf(fp, "\n");
  fprintf(fp, "void\n");
  fprintf(fp, "%s_debug(const struct %s *packet)", elist->name, elist->name);

}

/* Generate debug function. */
static void
exp_generate_debug(struct exp_list *elist) {
  struct exp *exp;

  /* Header output. */
  exp_generate_debug_function(elist, fp_header);
  fprintf(fp_header, ";\n");

  /* Source output. */
  exp_generate_debug_function(elist, fp_source);
  fprintf(fp_source, "\n");
  fprintf(fp_source, "{\n");
  fprintf(fp_source, "  /* Debug packet. */\n");
  fprintf(fp_source, "  lagopus_msg_debug(1, \"%s\\n\");\n", elist->name);
  fprintf(fp_source, "  if (packet != NULL) {\n");
  TAILQ_FOREACH(exp, &elist->tailq, entry) {
    enum decl_type type;

    /* Look up declaration type. */
    type = decl_type_lookup(exp->decl.type);

    /* In case of simple declaration. */
    if (exp->type == EXP_TYPE_DECLARATION) {
      switch (type) {
        case DECL_TYPE_CHAR:
        case DECL_TYPE_UCHAR:
          fprintf(fp_source, "    lagopus_msg_debug(1, \" %s: %%u\\n\", packet->%s);\n",
                  exp->decl.name, exp->decl.name);
          break;
        case DECL_TYPE_WORD:
          fprintf(fp_source, "    lagopus_msg_debug(1, \" %s: %%u\\n\", packet->%s);\n",
                  exp->decl.name, exp->decl.name);
          break;
        case DECL_TYPE_LONG:
          fprintf(fp_source, "    lagopus_msg_debug(1, \" %s: %%u\\n\", packet->%s);\n",
                  exp->decl.name, exp->decl.name);
          break;
        case DECL_TYPE_LLONG:
          fprintf(fp_source,
                  "    lagopus_msg_debug(1, \" %s: %%\"PRIu64\"\\n\", packet->%s);\n",
                  exp->decl.name, exp->decl.name);
          break;
        case DECL_TYPE_UNKNOWN:
          fprintf(stderr, "Error: unknown type %s\n", exp->decl.type);
          exit (1);
          break;
      }
    } else if (exp->type == EXP_TYPE_STRUCT_DECLARATION) {
#if 0 /* Do not generate nested structure debug. */
      fprintf(fp_source, "    lagopus_msg_debug(1, \" struct %s\\n\");\n",
              exp->decl.type);
      fprintf(fp_source, "    %s_debug (&packet->%s);\n",
              exp->decl.type, exp->decl.name);
      fprintf(fp_source, "    lagopus_msg_debug(1, \"\\n\");\n");
#endif
    } else {
      /* Array debug. */
      fprintf(fp_source, "    lagopus_msg_debug(1, \" %s: \\n\");\n",
              exp->decl.name);
      fprintf(fp_source, "    {\n");
      fprintf(fp_source, "      int i;\n");
      fprintf(fp_source, "      for (i = 0; i < %d; i++)\n",
              exp->decl.array_size);
      if (type == DECL_TYPE_CHAR)
        fprintf(fp_source,
                "        lagopus_msg_debug(1, \"%%c\\n\", packet->%s[i]);\n",
                exp->decl.name);
      else if (type == DECL_TYPE_UCHAR ||
               type == DECL_TYPE_WORD ||
               type == DECL_TYPE_LONG)
        fprintf(fp_source,
                "        lagopus_msg_debug(1, \"%%u \\n\", packet->%s[i]);\n",
                exp->decl.name);
      else if (type == DECL_TYPE_LLONG)
        fprintf(fp_source,
                "        lagopus_msg_debug(1, \"%%llu \\n\", packet->%s[i]);\n",
                exp->decl.name);
      fprintf(fp_source, "      lagopus_msg_debug(1, \"\\n\");\n");
      fprintf(fp_source, "    }\n");
    }
  }
  fprintf(fp_source, "  } else {\n");
  fprintf(fp_source, "    lagopus_msg_debug(1, \"NULL\\n\");\n");
  fprintf(fp_source, "  }\n");
  fprintf(fp_source, "}\n");
}

/* Generate trace function. */
static void
exp_generate_trace_function(struct exp_list *elist, FILE *fp) {
  fprintf(fp, "lagopus_result_t\n");
  fprintf(fp,
          "%s_trace(const void *pac, size_t pac_size, char *str, size_t max_len)",
          elist->name);
}

/* Generate doxygen for trace function. */
static void
exp_generate_doxygen_trace_function(struct exp_list *elist, FILE *fp) {
  fprintf(fp, "/**\n");
  fprintf(fp, " * Create trace string for \\e %s.\n", elist->name);
  fprintf(fp, " *\n");
  fprintf(fp, " *     @param[in]\tpac\tA pointer to \\e %s structure.\n",
          elist->name);
  fprintf(fp, " *     @param[in]\tpac_size\tSize of \\e %s structure.\n",
          elist->name);
  fprintf(fp, " *     @param[out]\tstr\tA pointer to \\e trace string.\n");
  fprintf(fp, " *     @param[in]\tmax_len\tMax length.\n");
  fprintf(fp, " *\n");
  fprintf(fp, " *     @retval\tLAGOPUS_RESULT_OK\tSucceeded.\n");
  fprintf(fp, " *     @retval\tLAGOPUS_RESULT_OUT_OF_RANGE\tOver max length.\n");
  fprintf(fp, " *     @retval\tLAGOPUS_RESULT_ANY_FAILURES\tFailed.\n");
  fprintf(fp, " */\n");
}

/* Generate trace function. */
static void
exp_generate_trace(struct exp_list *elist) {
  struct exp *exp;

  /* Header output. */
  fprintf(fp_header, "\n");
  exp_generate_doxygen_trace_function(elist, fp_header);
  exp_generate_trace_function(elist, fp_header);
  fprintf(fp_header, ";\n");

  /* Source output. */
  fprintf(fp_source, "\n");
  exp_generate_trace_function(elist, fp_source);
  fprintf(fp_source, "\n");
  fprintf(fp_source, "{\n");
  fprintf(fp_source, "  int n;\n");
  fprintf(fp_source, "  struct %s *packet = (struct %s *) pac;\n",
          elist->name, elist->name);
  fprintf(fp_source, "\n");
  fprintf(fp_source, "  if (pac_size == sizeof(struct %s)) {\n", elist->name);
  fprintf(fp_source, "    /* Trace packet. */\n");
  fprintf(fp_source, "    TRACE_STR_ADD(n, str, max_len, \"%s [\");\n",
          elist->name);
  fprintf(fp_source, "    if (packet != NULL) {\n");
  TAILQ_FOREACH(exp, &elist->tailq, entry) {
    enum decl_type type;

    /* Look up declaration type. */
    type = decl_type_lookup(exp->decl.type);

    /* In case of simple declaration. */
    if (exp->type == EXP_TYPE_DECLARATION) {
      switch (type) {
        case DECL_TYPE_CHAR:
        case DECL_TYPE_UCHAR:
          fprintf(fp_source,
                  "      TRACE_STR_ADD(n, str, max_len, \" %s: %%u,\", packet->%s);\n",
                  exp->decl.name, exp->decl.name);
          break;
        case DECL_TYPE_WORD:
          fprintf(fp_source,
                  "      TRACE_STR_ADD(n, str, max_len, \" %s: %%u,\", packet->%s);\n",
                  exp->decl.name, exp->decl.name);
          break;
        case DECL_TYPE_LONG:
          fprintf(fp_source,
                  "      TRACE_STR_ADD(n, str, max_len, \" %s: %%u,\", packet->%s);\n",
                  exp->decl.name, exp->decl.name);
          break;
        case DECL_TYPE_LLONG:
          fprintf(fp_source,
                  "      TRACE_STR_ADD(n, str, max_len, \" %s: %%\"PRIu64\",\", packet->%s);\n",
                  exp->decl.name, exp->decl.name);
          break;
        case DECL_TYPE_UNKNOWN:
          fprintf(stderr, "Error: unknown type %s\n", exp->decl.type);
          exit (1);
          break;
      }
    } else if (exp->type == EXP_TYPE_STRUCT_DECLARATION) {
      /* Do not generate nested structure trace. */
    } else {
      /* Array debug. */
      fprintf(fp_source, "      if (ARRAY_NOT_EMPTY(%d)) {\n", exp->decl.array_size);
      fprintf(fp_source, "        TRACE_STR_ADD(n, str, max_len, \" %s: \");\n",
              exp->decl.name);
      fprintf(fp_source, "        for (int i = 0; i < %d; i++) {\n",
              exp->decl.array_size);
      if (type == DECL_TYPE_CHAR)
        fprintf(fp_source,
                "          TRACE_STR_ADD(n, str, max_len, \"%%c\", packet->%s[i]);\n",
                exp->decl.name);
      else if (type == DECL_TYPE_UCHAR ||
               type == DECL_TYPE_WORD ||
               type == DECL_TYPE_LONG)
        fprintf(fp_source,
                "          TRACE_STR_ADD(n, str, max_len, \"%%u \", packet->%s[i]);\n",
                exp->decl.name);
      else if (type == DECL_TYPE_LLONG)
        fprintf(fp_source,
                "          TRACE_STR_ADD(n, str, max_len, \"%%llu \", packet->%s[i]);\n",
                exp->decl.name);
      fprintf(fp_source, "        }\n");
      fprintf(fp_source, "        TRACE_STR_ADD(n, str, max_len, \",\");\n");
      fprintf(fp_source, "      }\n");
    }
  }
  fprintf(fp_source, "    } else {\n");
  fprintf(fp_source, "      TRACE_STR_ADD(n, str, max_len, \"NULL\");\n");
  fprintf(fp_source, "    }\n");
  fprintf(fp_source, "    TRACE_STR_ADD(n, str, max_len, \"]\");\n");
  fprintf(fp_source, "  } else {\n");
  fprintf(fp_source, "    return LAGOPUS_RESULT_OUT_OF_RANGE;\n");
  fprintf(fp_source, "  }\n");
  fprintf(fp_source, "\n");
  fprintf(fp_source, "  return LAGOPUS_RESULT_OK;\n");
  fprintf(fp_source, "}\n");
}

static void
exp_generate_enum(struct exp_list *elist) {
  struct exp *exp;

  fprintf(fp_header, "\n");
  fprintf(fp_header, "const char *\n");
  fprintf(fp_header, "%s_str(enum %s val);\n", elist->name, elist->name);

  fprintf(fp_source, "\n");
  fprintf(fp_source, "const char *\n");
  fprintf(fp_source, "%s_str(enum %s val)\n", elist->name, elist->name);
  fprintf(fp_source, "{\n");
  fprintf(fp_source, "  switch (val) {\n");

  TAILQ_FOREACH(exp, &elist->tailq, entry) {
    fprintf(fp_source, "    case %s:\n", exp->decl.name);
    fprintf(fp_source, "      return \"%s\";\n", exp->decl.name);
  }

  fprintf(fp_source, "    default:\n");
  fprintf(fp_source, "      return \"Unknown\";\n");
  fprintf(fp_source, "  }\n");
  fprintf(fp_source, "  return \"\";\n");
  fprintf(fp_source, "}\n");
}

/* Create statement. */
struct exp *
create_statement(void) {
  if (! top) {
    return NULL;
  }

  if (top->type == EXP_TYPE_ENUM) {
    exp_generate_enum(&top->elist);
  } else {
    exp_generate_encoder(&top->elist);
    exp_generate_list_encoder(&top->elist);
    exp_generate_decoder(&top->elist);
    exp_generate_sneak_decoder(&top->elist);
    exp_generate_debug(&top->elist);
    exp_generate_trace(&top->elist);
  }
  exp_free(top);

  top = NULL;

  return NULL;
}

/* Create top expression. */
static struct exp *
create_list_expression(void) {
  struct exp *exp;

  exp = exp_new();
  if (exp) {
    exp->type = EXP_TYPE_LIST;
    TAILQ_INIT(&exp->elist.tailq);
  }

  return exp;
}

/* Create struct expression. */
struct exp *
create_struct_expression(char *struct_name) {
  /* Top expression. */
  if (! top) {
    top = create_list_expression();
  }

  /* Struct name. */
  top->elist.name = strdup(struct_name);

  return top;
}

/* Create enum expression. */
struct exp *
create_enum_expression(char *enum_name) {
  /* Top expression. */
  if (! top) {
    top = create_list_expression();
  }

  /* Enum name. */
  top->type = EXP_TYPE_ENUM;
  top->elist.name = strdup(enum_name);

  return top;
}

/* Create declaration and push to the expression. */
struct exp *
create_decl_expression(char *type, char *name) {
  struct exp *exp;

  /* Top. */
  if (! top) {
    top = create_list_expression();
  }

  /* Allocate new expression. */
  exp = exp_new();
  if (exp == NULL) {
    return NULL;
  }

  /* Assign declaration. */
  exp->type = EXP_TYPE_DECLARATION;
  exp->decl.type = strdup(type);
  exp->decl.name = strdup(name);

  TAILQ_INSERT_TAIL(&top->elist.tailq, exp, entry);

  return exp;
}

struct exp *
create_decl_array_expression(char *type, char *name, int array_size) {
  struct exp *exp;

  /* Top. */
  if (! top) {
    top = create_list_expression();
  }

  /* Allocate new expression. */
  exp = exp_new();
  if (exp == NULL) {
    return NULL;
  }

  /* Assign declaration. */
  exp->type = EXP_TYPE_ARRAY_DECLARATION;
  exp->decl.type = strdup(type);
  exp->decl.name = strdup(name);
  exp->decl.array_size = array_size;
  TAILQ_INSERT_TAIL(&top->elist.tailq, exp, entry);

  return exp;
}

struct exp *
create_decl_array_id_expression(char *type, char *name, char *id) {
  int array_size = 0;

  if (strcmp(id, "OFP_ETH_ALEN") == 0) {
    array_size = OFP_ETH_ALEN;
  } else if (strcmp(id, "OFP_MAX_PORT_NAME_LEN") == 0) {
    array_size = OFP_MAX_PORT_NAME_LEN;
  } else if (strcmp(id, "OFPMT_STANDARD_LENGTH") == 0) {
    array_size = OFPMT_STANDARD_LENGTH;
  } else if (strcmp(id, "DESC_STR_LEN") == 0) {
    array_size = DESC_STR_LEN;
  } else if (strcmp(id, "SERIAL_NUM_LEN") == 0) {
    array_size = SERIAL_NUM_LEN;
  } else if (strcmp(id, "OFP_MAX_TABLE_NAME_LEN") == 0) {
    array_size = OFP_MAX_TABLE_NAME_LEN;
  } else {
    fprintf(stderr, "Unknown ID : %s\n", id);
    return NULL;
  }
  return create_decl_array_expression(type, name, array_size);
}

struct exp *
create_decl_struct_expression(char *type, char *name) {
  struct exp *exp;

  /* Top. */
  if (! top) {
    top = create_list_expression();
  }

  /* Allocate new expression. */
  exp = exp_new();
  if (exp == NULL) {
    return NULL;
  }

  /* Assign declaration. */
  exp->type = EXP_TYPE_STRUCT_DECLARATION;
  exp->decl.type = strdup(type);
  exp->decl.name = strdup(name);
  TAILQ_INSERT_TAIL(&top->elist.tailq, exp, entry);

  return exp;
}

struct exp *
create_decl_array_struct_expression(char *type, char *name, int array_size) {
  struct exp *exp;

  /* Top. */
  if (! top) {
    top = create_list_expression();
  }

  /* Allocate new expression. */
  exp = exp_new();
  if (exp == NULL) {
    return NULL;
  }

  /* Assign declaration. */
  exp->type = EXP_TYPE_ARRAY_STRUCT_DECLARATION;
  exp->decl.type = strdup(type);
  exp->decl.name = strdup(name);
  exp->decl.array_size = array_size;
  TAILQ_INSERT_TAIL(&top->elist.tailq, exp, entry);

  return exp;
}

struct exp *
create_decl_enum_expression(char *name) {
  struct exp *exp;

  /* Top. */
  if (! top) {
    top = create_list_expression();
  }

  /* Allocate new expression. */
  exp = exp_new();
  if (exp == NULL) {
    return NULL;
  }

  /* Assign declaration. */
  exp->type = EXP_TYPE_ENUM_DECLARATION;
  exp->decl.name = strdup(name);
  TAILQ_INSERT_TAIL(&top->elist.tailq, exp, entry);

  return exp;
}
