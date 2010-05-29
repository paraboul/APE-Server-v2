#ifndef _STRING_H_
#define _STRING_H_

#include "buffer.h"


typedef enum {
	ISO88591,
	UTF8
} string_encoding;


typedef struct {
	buffer b;
	int len;
	string_encoding encoding;
} string;

#endif