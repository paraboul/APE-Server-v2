#ifndef __APE_COMMON_PATTERN_H
#define __APE_COMMON_PATTERN_H


const char *PATTERN_LB = 		"{";
const char *PATTERN_RB =	"}";
const char *PATTERN_COLON = 	":";
const char *PATTERN_DQUOTE = 	"\"";
const char *PATTERN_COMMA = 	",";

const char *PATTERN_HTTP_200 =	"HTTP/1.1 200 OK\r\n";

const unsigned char PATTERN_ERR_BAD_JSON[] = "[\"ERR\",0,[\"BAD_JSON\"]]";
const unsigned char PATTERN_ERR_INTERNAL[] = "[\"ERR\",0,[\"INTERNAL_ERROR\"]]";

#endif

