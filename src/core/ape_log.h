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

/* ape_log.h */

#ifndef _APE_LOG_H
#define _APE_LOG_H

#if 0

#include "main.h"

typedef enum {
	APE_DEBUG 	= 0x01,
	APE_WARN 	= 0x02,
	APE_ERR 	= 0x04,
	APE_INFO	= 0x08
} ape_log_lvl_t;

void ape_log_init(ape_global *ape);
void ape_log(ape_log_lvl_t lvl, const char *file, unsigned long int line, ape_global *ape, char *buf, ...);

#endif

#endif

