#include "string.h"

#include <stdlib.h>
#include <string.h>

string *string_new(const char *str, size_t len, string_encoding encoding)
{
	//buffer *b;
	
	switch(encoding) {
		case UTF8:
			
			break;
		case ISO88591:
		default:
			
			break;
	}
}

void string_update_len(string *str)
{
	int pos;
	unsigned int c;
	char *s;
	
	if (str->encoding == ISO88591) {
		str->len = strlen(str->b.data);
		return;
	}
	
	str->len = 0;
	pos = str->b.used;
	s = str->b.data;
	
	while (pos > 0) {
		c = (unsigned char)(*s);
		
		if (c >= 0xf0) { /* four bytes encoded, 21 bits */
			str->len += 4;
			s += 4;
			pos -= 4;
		} else if (c >= 0xe0) { /* three bytes encoded, 16 bits */
			str->len += 3;
			s += 3;
			pos -= 3;
		} else if (c >= 0xc0) { /* two bytes encoded, 11 bits */
			str->len += 2;
			s += 2;
			pos -= 2;
		} else {
			str->len++;
			s++;
			pos--;
		}
	}
}

