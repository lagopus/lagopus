/*
 * Copyright 2014-2015 Nippon Telegraph and Telephone Corporation.
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


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cnode.h"
#include "validate.h"

#define WHITE_SPACE(C)     ((C) == ' ' || (C) == '\t' || (C) == '\n')
#define DELIMITER(C)       (WHITE_SPACE(C) || (C) == '\0')

/* Match to keyword. */
char *
keyword_match(char *cp, char *name, enum match_type *type) {
  while (!DELIMITER(*cp) && !DELIMITER(*name) && *cp == *name) {
    cp++, name++;
  }

  if (!DELIMITER(*cp)) {
    *type = NONE_MATCH;
    return cp;
  }

  if (!DELIMITER(*name)) {
    *type = PARTIAL_MATCH;
  } else {
    *type = KEYWORD_MATCH;
  }

  return cp;
}

/* Match to word. */
char *
word_match(char *cp, enum match_type *type) {
  int quote = 0;
  int backslash = 0;

  if (*cp == '\0') {
    *type = NONE_MATCH;
  }

  while (*cp != '\0') {
    if (backslash) {
      backslash = 0;
    } else {
      if (*cp == '\\') {
        backslash = 1;
      } else {
        if (quote) {
          if (*cp == '"') {
            quote = 0;
          }
        } else {
          if (*cp == '"') {
            quote = 1;
          } else if (DELIMITER(*cp)) {
            break;
          }
        }
      }
    }
    cp++;
  }

  *type = WORD_MATCH;

  return cp;
}

/* IPv4 address match routine.  When the address is valid but not
   fully described, return incomplete match.  */
char *
ipv4_match(char *cp, enum match_type *type) {
  char *sp;
  int dots = 0;
  int nums = 0;
  char buf[4];
  *type = NONE_MATCH;

  for (;;) {
    sp = cp;

    /* Find the next dot - check the number is OK. */
    while (!DELIMITER(*cp)) {
      if (*cp == '.') {
        if (dots >= 3) {
          return cp;
        }
        dots++;
        break;
      }
      if (!isdigit((int) *cp)) {
        return cp;
      }
      cp++;
    }
    /* Check the length of the number is OK - convert. */
    if (cp - sp > 3) {
      return sp;
    } else if (cp-sp == 0) {
      /* A dot at the beginning or an empty string */
      return cp;
    } else {
      /* Convert the number. */
      memset(buf, 0, sizeof(buf));
      strncpy(buf, sp, (size_t)(cp - sp));
      if (strtol(buf, NULL, 10) > 255) {
        return sp;
      }
      nums++;
    }
    if (DELIMITER(*cp)) {
      break;
    }
    cp++; /* skip the dot */
  }
  if (nums < 4) {
    *type = PARTIAL_MATCH;
    return cp;
  }
  *type = IPV4_MATCH;
  return cp;
}

char *
ipv4_prefix_match(char *cp, enum match_type *type) {
  char *p;
  char *sp;

  *type = NONE_MATCH;

  /* Check the string includes '/'.  */
  p = strchr(cp, '/');

  /* This is partial input.  */
  if (!p) {
    /* IPv4 address match.  */
    cp = ipv4_match(cp, type);

    /* When return value is NULL.  */
    if (*type == NONE_MATCH) {
      return cp;
    } else {
      /* It is incomplete match.  */
      *type = PARTIAL_MATCH;
      return cp;
    }
  }

  /* '/' is in the string.  Perform matching to IPv4 address part.  */
  *p = ' ';
  cp = ipv4_match(cp, type);
  *p = '/';

  /* IPv4 part must be complete address.  */
  if (!cp) {
    return NULL;
  }

  if (*type != IPV4_MATCH) {
    return cp;
  }

  /* Check mask length.  */
  cp = p + 1;

  if (DELIMITER(*cp)) {
    *type = PARTIAL_MATCH;
    return cp;
  }

  sp = cp;
  while (!DELIMITER(*cp)) {
    if (!isdigit((int) *cp)) {
      *type = NONE_MATCH;
      return cp;
    }
    cp++;
  }

  if (strtol(sp, NULL, 10) > 32) {
    *type = NONE_MATCH;
    return cp;
  }

  *type = IPV4_PREFIX_MATCH;
  return cp;
}

char *
ipv6_match(char *cp, enum match_type *type) {
  struct in6_addr addr;
#define INET6_BUFSIZ        51
  char buf[INET6_BUFSIZ + 1];
  size_t len;
  int ret;
  *type = NONE_MATCH;

  /* Delimiter check.  */
  if (DELIMITER(*cp)) {
    return NULL;
  }

  /* Character check.  */
  len = strspn(cp, "0123456789abcdefABCDEF:.%");

  if (!DELIMITER(cp[len])) {
    return NULL;
  }

  if (len > INET6_BUFSIZ) {
    return NULL;
  }

  memcpy(buf, cp, len);
  buf[len] = '\0';

  ret = inet_pton(AF_INET6, buf, &addr);
  if (!ret) {
    *type = PARTIAL_MATCH;
  } else {
    *type = IPV6_MATCH;
  }

  return cp + len;
}

char *
ipv6_prefix_match(char *cp, enum match_type *type) {
  char *p;
  char *sp;
  *type = NONE_MATCH;

  /* Check the string includes '/'.  */
  p = strchr(cp, '/');

  /* This is partial input.  */
  if (!p) {
    /* IPv4 address match.  */
    cp = ipv6_match(cp, type);

    /* When return value is NULL.  */
    if (*type == NONE_MATCH) {
      return cp;
    } else {
      /* It is incomplete match.  */
      *type = PARTIAL_MATCH;
      return cp;
    }
  }

  /* '/' is in the string.  Perform matching to IPv4 address part.  */
  *p = ' ';
  cp = ipv6_match(cp, type);
  *p = '/';

  /* IPv6 part must be complete address.  */
  if (!cp || *type != IPV6_MATCH) {
    return NULL;
  }

  /* Check mask length.  */
  cp = p + 1;

  if (DELIMITER(*cp)) {
    *type = PARTIAL_MATCH;
    return cp;
  }

  sp = cp;
  while (!DELIMITER(*cp)) {
    if (!isdigit((int) *cp)) {
      *type = NONE_MATCH;
      return NULL;
    }
    cp++;
  }

  if (strtol(sp, NULL, 10) > 128) {
    *type = NONE_MATCH;
    return cp;
  }

  *type = IPV6_PREFIX_MATCH;
  return cp;
}

/* Ethernet MAC address match function.  */
char *
mac_address_match(char *cp, enum match_type *type) {
  char *sp;
  int dots = 0;
  int nums = 0;
  *type = NONE_MATCH;

  sp = cp;

  /* Loop until we see delimiter.  */
  while (!DELIMITER(*cp)) {
    if (*cp == ':') {
      if (nums == 0) {
        return cp;
      }

      if (dots >= 5) {
        return cp;
      }

      if (*(cp + 1) == ':') {
        return cp + 1;
      }

      dots++;
      cp++;
      sp = cp;
    } else {
      /* If the character is not alnum, return.  */
      if (!isalnum((int) *cp)) {
        return cp;
      }

      /* If the character orrur more than two, it it error.  */
      if (cp - sp >= 2) {
        return cp;
      }

      /* Increment the character pointer.  */
      nums++;
      cp++;
    }
  }

  /* Matched, check this is exact match or not.  */
  if (dots != 5 || nums != 12) {
    *type = PARTIAL_MATCH;
  } else {
    *type = MAC_ADDRESS_MATCH;
  }

  return cp;
}

/* Convert string to uint64_t. */
char *
str2int64(char *str, int64_t *val) {
  int digit;
  int64_t limit;
  int minus = 0;
  int64_t max = INT64_MAX;
  int64_t total = 0;
  char *cp = str;

  /* Sanify check. */
  if (str == NULL || val == NULL) {
    return NULL;
  }

  /* '+' and '-' check. */
  if (*cp == '+') {
    cp++;
  } else if (*cp == '-') {
    /* No negative value handle for now.*/
    cp++;
    minus = 1;
    //max = max / 2 + 1;
  }

  limit = max / 10;

  /* Parse digits. */
  while (isdigit((int)*cp)) {
    digit = *cp++ - '0';

    if (total > limit) {
      return NULL;
    }

    total = (total * 10) + (int64_t)digit;
  }

  /* String must be terminated white space or '\0'.  */
  if (!DELIMITER(*cp)) {
    return NULL;
  }

  /* Parse success.  */
  *val = minus ? -total : total;

  return cp;
}

#if 0
/* Convert string to uint64_t. */
static char *
str2uint64(char *str, uint64_t *val) {
  int digit;
  uint64_t limit;
  int minus = 0;
  uint64_t max = UINT64_MAX;
  uint64_t total = 0;
  char *cp = str;

  /* Sanify check. */
  if (str == NULL || val == NULL) {
    return NULL;
  }

  /* '+' and '-' check. */
  if (*cp == '+') {
    cp++;
  } else if (*cp == '-') {
#if 0 /* No negative value handle for now.*/
    cp++;
    minus = 1;
    max = max / 2 + 1;
#endif
  }

  limit = max / 10;

  /* Parse digits. */
  while (isdigit((int)*cp)) {
    digit = *cp++ - '0';

    if (total > limit) {
      return NULL;
    }

    total = (total * 10) + (uint64_t)digit;
  }

  /* String must be terminated white space or '\0'.  */
  if (!DELIMITER(*cp)) {
    return NULL;
  }

  /* Parse success.  */
  *val = minus ? -total : total;

  return cp;
}
#endif

/* Integer range match function. */
char *
range_match(char *cp, struct cnode *cnode, enum match_type *type) {
  char *sp;
  int64_t val;
  *type = NONE_MATCH;

  /* Remember the start pointer. */
  sp = cp;

  /* Convert string to uint64_t. */
  cp = str2int64(cp, &val);
  if (!cp) {
    return sp;
  }

  /* Min value check. */
  if (val < cnode->min) {
    return cp;
  }

  /* Max value check. */
  if (val > cnode->max) {
    return cp;
  }

  /* Current character must be delimiter. */
  if (!DELIMITER(*cp)) {
    return sp;
  }

  /* Success. */
  *type = RANGE_MATCH;

  return cp;
}
