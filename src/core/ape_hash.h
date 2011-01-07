/*
  Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011  Anthony Catel <a.catel@weelya.com>

  This file is part of APE Server.
  APE is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  APE is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with APE ; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef _APE_HASH_H
#define _APE_HASH_H

#include <stdint.h>

#define HACH_TABLE_MAX 8192

typedef struct _ape_htable
{
	struct _ape_htable_item *first;
	struct _ape_htable_item **table;
} ape_htable_t;


typedef struct _ape_htable_item
{
	char *key;
	void *addrs;
	struct _ape_htable_item *next;
	
	struct _ape_htable_item *lnext;
	struct _ape_htable_item *lprev;
	
} ape_htable_item_t;

ape_htable_t *hashtbl_init();

void hashtbl_free(ape_htable_t *htbl);
void *hashtbl_seek(ape_htable_t *htbl, const char *key, int key_len);
void hashtbl_erase(ape_htable_t *htbl, const char *key, int key_len);
void hashtbl_append(ape_htable_t *htbl, const char *key, int key_len, void *structaddr);
uint32_t ape_hash_str(const void *key, int len);
unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed );

#endif
