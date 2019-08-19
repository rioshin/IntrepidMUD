/*
 * IntrepidMUD
 * MUD Server
 * The main MUD server itself
 * ---------------------------------------------------------------------------
 * Copyright 2012-2019 by Mikael Segercrantz, Dan Griffiths and Dave Etheridge
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the license, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/mudconfig.h"

#include <sys/types.h>
#include <netinet/in.h>

#include "../include/proto.h"

char uncompact_table[7][16] = {
  {0, ' ', '\n', 'a', 'e', 'h', 'i', 'n', 'o', 's', 't', 1, 2, 3, 4, 5},
  {'b', 'c', 'd', 'f', 'g', 'j', 'k', 'l', 'm', 'p', 'q', 'r', 'u', 'v',
   'w', 'x'},
  {'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
   'M', 'N'},
  {'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '"',
   '#', '@'},
  {'~', '&', 39, '(', ')', '*', '+', ',', '-', '.', '/', '0', '1', '2',
   '3', '4'},
  {'5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '[', '\\', ']',
   '^', 6},
  {'`', '_', '{', '|', '}', '%', '$', -1, 0, 0, 0, 0, 0, 0, 0, 0}
};

char compact_table[128] = {
  1, 60, 61, 62, 102, 101, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
  77, 78, 79, 80,
  81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 63, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46,
  47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 91, 92, 93, 94, 97,
  96, 3, 16, 17, 18, 4,
  19, 20, 5, 6, 21, 22, 23, 24, 7, 8, 25, 26, 27, 9, 10, 28, 29, 30, 31,
  32, 33, 98, 99, 100, 64
};

unsigned char *store_nibble(unsigned char *dest, int32 n)
{
  static int32 toggle = 0;

  if (toggle)
  {
    *dest++ |= (char)n;
    toggle = 0;
  }
  else
  {
    if (!n)
    {
      *dest++ = null_chr;
      return (unsigned char *)dest;
    }
    n <<= 4;
    *dest = (char)n;
    toggle = 1;
  }

  return (unsigned char *)dest;
}

unsigned char *get_nibble(int32 * n, unsigned char *source)
{
  static int32 toggle = 0;

  if (toggle)
  {
    *n = ((int32) * source++) & 15;
    toggle = 0;
  }
  else
  {
    *n = (((int32) * source) >> 4) & 15;
    if (!*n)
      source++;
    else
      toggle = 1;
  }

  return source;
}

unsigned char *get_string(char *dest, unsigned char *source)
{
  char c = 1;
  int32 table = 0;
  int32 n;

  for (; c; table = 0)
  {
    c = 1;
    while (c && c < 7)
    {
      source = get_nibble(&n, source);
      c = uncompact_table[table][n];
      if (c && c < 7)
        table = (int32) c;
    }
    *dest++ = c;
  }

  return source;
}

unsigned char *store_string(unsigned char *dest, char *source)
{
  int32 n, row, tmp = 1;

  for (; tmp; source++)
  {
    tmp = (pint) * source;
    switch (tmp)
    {
    case 0:
      row = 0;
      n = 0;
      break;

    case '\n':
      row = 0;
      n = 2;
      break;

    case -1:
      row = 6;
      n = 7;
      break;

    default:
      row = (compact_table[tmp - 32]) >> 4;
      n = (compact_table[tmp - 32]) & 15;
      break;
    }
    switch (row)
    {
    case 0:
      dest = store_nibble(dest, n);
      break;

    case 1:
      dest = store_nibble(dest, 11);
      dest = store_nibble(dest, n);
      break;

    case 2:
      dest = store_nibble(dest, 12);
      dest = store_nibble(dest, n);
      break;

    case 3:
      dest = store_nibble(dest, 13);
      dest = store_nibble(dest, n);
      break;

    case 4:
      dest = store_nibble(dest, 14);
      dest = store_nibble(dest, n);
      break;

    case 5:
      dest = store_nibble(dest, 15);
      dest = store_nibble(dest, n);
      break;

    case 6:
      dest = store_nibble(dest, 15);
      dest = store_nibble(dest, 15);
      dest = store_nibble(dest, n);
      break;
    }
  }

  return (unsigned char *)dest;
}

unsigned char *store_word(unsigned char *dest, int32 source)
{
  *dest++ = (source >> 8) & 255;
  *dest++ = source & 255;

  return (unsigned char *)dest;
}

unsigned char *store_int32(unsigned char *dest, int32 source)
{
  int32 i;
  union
  {
    char c[4];
    int32 i;
  } u;

  u.i = htonl(source);
  for (i = 0; i < 4; i++)
    *dest++ = u.c[i];

  return (unsigned char *)dest;
}

unsigned char *store_int64(unsigned char *dest, int64 source)
{
  int32 i1, i2;

  i1 = (int32) (source & 0xffffffff);
  i2 = (int32) ((source >> 32) & 0xffffffff);

  dest = store_int32(dest, i1);
  return store_int32(dest, i2);
}

unsigned char *get_word(pint * dest, char *source)
{
  union
  {
    char c[4];
    int32 i;
  }
  u;

  u.i = 0;
  u.c[3] = *source++;
  u.c[4] = *source++;
  *dest = ntohl(u.i);

  return (unsigned char *)source;
}

unsigned char *get_int32(int32 * dest, unsigned char *source)
{
  int32 i;
  union
  {
    char c[4];
    int32 i;
  }
  u;

  for (i = 0; i < 4; i++)
    u.c[i] = *source++;
  *dest = ntohl(u.i);

  return (unsigned char *)source;
}

unsigned char *get_int64(int64 * dest, unsigned char *source)
{
  int32 i1, i2;

  source = (unsigned char *)get_int32(&i1, source);
  source = (unsigned char *)get_int32(&i2, source);

  *dest = (((int64) i2) << 32) & (int64) i1;

  return (unsigned char *)source;
}

unsigned char *get_int32_safe(int32 * no, unsigned char *r, file data)
{
  if ((no) && (r))
  {
    if (((pint) r - (pint) data.where) < data.length)
      return get_int32(no, r);
    else
    {
      *no = 0;

      return (unsigned char *)r;
    }
  }
  else if (no)
    *no = 0;

  return (unsigned char *)r;
}

unsigned char *get_int64_safe(int64 * no, unsigned char *r, file data)
{
  if ((no) && (r))
  {
    if (((pint) r - (pint) data.where) < data.length)
      return get_int64(no, r);
    else
    {
      *no = 0;

      return (unsigned char *)r;
    }
  }
  else if (no)
    *no = 0;

  return (unsigned char *)r;
}

unsigned char *get_string_safe(char *str, unsigned char *r, file data)
{
  if ((str) && (r))
  {
    if (((pint) r - (pint) data.where) < data.length)
      return get_string(str, r);
    else
    {
      *str = null_chr;

      return (unsigned char *)r;
    }
  }
  else if (str)
    *str = null_chr;

  return (unsigned char *)r;
}

unsigned char *get_int32_safe_explicit(int32 * number,
                                       char *compressed_data,
                                       const char *compressed_start,
                                       size_t compress_length)
{
  if ((number) && (compressed_data))
  {
    if (((size_t) compressed_data - (size_t) compressed_start) <
        compress_length)
      return get_int32(number, (unsigned char *)compressed_data);
    else
    {
      *number = 0;

      return (unsigned char *)compressed_data;
    }
  }
  else if (number)
    *number = 0;

  return (unsigned char *)compressed_data;
}

unsigned char *get_int64_safe_explicit(int64 * number,
                                       char *compressed_data,
                                       const char *compressed_start,
                                       size_t compress_length)
{
  if ((number) && (compressed_data))
  {
    if (((size_t) compressed_data - (size_t) compressed_start) <
        compress_length)
      return get_int64(number, (unsigned char *)compressed_data);
    else
    {
      *number = 0;

      return (unsigned char *)compressed_data;
    }
  }
  else if (number)
    *number = 0;

  return (unsigned char *)compressed_data;
}
