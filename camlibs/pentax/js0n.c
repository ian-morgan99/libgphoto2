// by jeremie miller - 2014
// public domain or MIT license, contributions/improvements welcome via github at https://github.com/quartzjer/js0n

#include <string.h> // one strncmp() is used to do key comparison, and a strlen(key) if no len passed in
#include "js0n.h"


// only at depth 1, track start pointers to match key/value
#define PUSH(i) if(depth == 1) { if(!index) { val = cur+i; }else{ if(klen && index == 1) start = cur+i; else index--; } }

// determine if key matches or value is complete
#define CAP(i) if(depth == 1) { if(val && !index) {*vlen = (size_t)((cur+i+1) - val); return val;}; if(klen && start) {index = (klen == (size_t)(cur-start) && strncmp(key,start,klen)==0) ? 0 : 2; start = 0;} }

typedef enum {
    STATE_STRUCT,      // Corresponds to gostruct
    STATE_BARE,        // Corresponds to gobare
    STATE_STRING,      // Corresponds to gostring
    STATE_UTF8_CONTINUE,// Corresponds to goutf8_continue
    STATE_ESC          // Corresponds to goesc
} JsonState;

// this makes a single pass across the json bytes, using a state machine to build an index and transition state
const char *js0n(const char *key, size_t klen,
				 const char *json, size_t jlen, size_t *vlen)
{
	const char *val = 0;
	const char *cur, *end, *start;
	size_t index = 1;
	int depth = 0;
	int utf8_remain = 0;
    JsonState currentState = STATE_STRUCT;

	if(!json || jlen <= 0 || !vlen) return 0;
	*vlen = 0;

	// no key is array mode, klen provides requested index
	if(!key)
	{
		index = klen;
		klen = 0;
	}else{
		if(klen <= 0) klen = strlen(key); // convenience
	}

    cur = json;
    start = json;
    end = cur + jlen;

    while (cur < end) {
        unsigned char ch = (unsigned char)*cur;
        int next_char = 1; // Flag to control if cur should be incremented

        switch (currentState) {
            case STATE_STRUCT:
                switch (ch) {
                    case '\t': case ' ': case '\r': case '\n': case ':': case ',':
                        // l_loop equivalent
                        break; // Character processed, cur will be incremented
                    case '"': // l_qup
                        PUSH(1);
                        currentState = STATE_STRING;
                        break;
                    case '[': case '{': // l_up
                        PUSH(0);
                        ++depth;
                        break;
                    case ']': case '}': // l_down
                        --depth;
                        CAP(0); // This might return from js0n
                        break;
                    case '-':
                    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
                    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
                    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
                    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
                    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
                    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': // l_bare
                        PUSH(0);
                        currentState = STATE_BARE;
                        break;
                    default: // l_bad
                        *vlen = cur - json;
                        return 0;
                }
                break;

            case STATE_BARE:
                switch (ch) {
                    case '\t': case ' ': case '\r': case '\n':
                    case ',': case ']': case '}': case ':': // l_unbare
                        CAP(-1); // This might return from js0n
                        currentState = STATE_STRUCT;
                        next_char = 0; // Re-process current character in new state
                        break;
                    default:
                        if (ch <= 31 || ch == 127) { // l_bad from gobare (0-31, 127)
                            *vlen = cur - json;
                            return 0;
                        }
                        if (ch >= 128 && ch <= 255) { // l_bad from gobare (128-255) (technically 127 is already covered)
                             *vlen = cur - json;
                             return 0;
                        }
                        // If not bad and not unbare, it's l_loop from gobare
                        break;
                }
                break;

            case STATE_STRING:
                switch (ch) {
                    case '\\': // l_esc
                        currentState = STATE_ESC;
                        break;
                    case '"': // l_qdown
                        CAP(-1); // This might return from js0n
                        currentState = STATE_STRUCT;
                        break;
                    default:
                        if ((ch <= 31) || ch == 127) { // l_bad from gostring (0-31, 127)
                            *vlen = cur - json;
                            return 0;
                        }
                        // UTF-8 checks from gostring
                        if (ch >= 192 && ch <= 223) { // l_utf8_2
                            currentState = STATE_UTF8_CONTINUE;
                            utf8_remain = 1;
                        } else if (ch >= 224 && ch <= 239) { // l_utf8_3
                            currentState = STATE_UTF8_CONTINUE;
                            utf8_remain = 2;
                        } else if (ch >= 240 && ch <= 247) { // l_utf8_4
                            currentState = STATE_UTF8_CONTINUE;
                            utf8_remain = 3;
                        } else if (ch >= 128 && ch <= 191) { // Stray UTF-8 continuation byte, bad. (l_bad from gostring)
                             *vlen = cur - json;
                             return 0;
                        } else if (ch >= 248) { // l_bad from gostring (for 248-255)
                             *vlen = cur - json;
                             return 0;
                        }
                        // Otherwise, it's l_loop from gostring (character is part of the string)
                        break;
                }
                break;

            case STATE_ESC:
                switch (ch) {
                    case '"': case '\\': case '/': case 'b':
                    case 'f': case 'n': case 'r': case 't': case 'u': // l_unesc
                        currentState = STATE_STRING;
                        break;
                    default: // l_bad from goesc
                        *vlen = cur - json;
                        return 0;
                }
                break;

            case STATE_UTF8_CONTINUE:
                if (ch >= 128 && ch <= 191) { // l_utf_continue
                    if (!--utf8_remain) {
                        currentState = STATE_STRING;
                    }
                    // currentState remains STATE_UTF8_CONTINUE if utf8_remain > 0
                } else { // l_bad from goutf8_continue
                    *vlen = cur - json;
                    return 0;
                }
                break;
        } // end switch(currentState)

        if (next_char) {
            cur++;
        }
    } // end while(cur < end)

	if(depth) *vlen = jlen; // incomplete json
	return 0; // If not returned from CAP earlier, means key not found or json incomplete
}
