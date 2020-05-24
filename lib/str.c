#include <string.h>
#include <stdlib.h>
#include <ctype.h>		/* For tolower() and toupper() function */
#include <stdbool.h>

#include "str.h"


int str_ltrim(char *line)
{
	int i, k, is_trimmed;

	is_trimmed = 0;
	for (i = k = 0; line[i]; ++i)
	{
		if (!is_trimmed && isspace(line[i]))
			continue;
		else
			is_trimmed = 1;

		line[k++] = line[i];
	}
	line[k] = '\0';

	return k;
}

int str_rtrim(char *line)
{
	int i, len;

	len = strlen(line);
	for (i = len - 1; i > 0; --i)
		if (!isspace(line[i]))
			break;

	line[++i] = '\0';

	return len - i;
}

int str_trim(char *line)
{
	return str_ltrim(line) + str_rtrim(line);
}

/**
 * Internal function used by find_next() function
 */
static const char *match(const char *s, const char *pattern)
{
    int i;
    for (i = 0; s[i] && pattern[i]; ++i)
    {
        // If current character doesn't match with current char of string s
        // then match not found
        if (s[i] != pattern[i])
            return NULL;
    }

    return (pattern[i] == '\0') ? &s[i - 1] : NULL;
}

bool str_find_next(const char *s, const char *pattern, struct Segment *seg)
{
    // Error checking
    if (s == NULL || s[0] == '\0')             return false;
    if (pattern == NULL || pattern[0] == '\0') return false;

    // End Match offset returned by match function 
    const char *match_off = NULL;

    // Start Offset of the given string from where next search will begin
    const char *start = s;

    // If find_next() is called after first found pattern then set start
    // position from last found position
    if (seg->start_off != NULL)
        start = (char *)seg->end_off + 1;

    for (const char *cp = start; *cp; ++cp)
    {
        // If first character of pattern matches then call match function
        // to match whole pattern and if match found then set offsets
        if (*cp == pattern[0])
        {
            if (match_off = match(cp, pattern))
            {
                // Start offset of the string s    offset is a mem address
                seg->start_off = cp;

                // End offset of the string s
                seg->end_off = match_off;
                return true;
            }
        }
    }

    return false;
}

char *str_replace(const char *s, const char *pattern, const char *substitue)
{
    // Error checking for valid string and pattern
    if (s == NULL || s[0] == '\0')             return false;
    if (pattern == NULL || pattern[0] == '\0') return false;
    if (substitue == NULL)	                   return false;

    char *end_off = NULL;
    struct Segment seg = {NULL};
    char *res = (char *)malloc(sizeof(char) * 256);

    int res_i, s_i, pattern_len;
    res_i = s_i = 0;
    pattern_len = strlen(pattern);
    while(str_find_next(s, pattern, &seg))
    {
        // copy
        for (const char *sp = &s[s_i]; sp != seg.start_off; ++sp)
        {
            res[res_i++] = s[s_i++];
        }

        // Copy the pattern to result string
        int i;
        for (i = 0; substitue[i]; ++i)
            res[res_i++] = substitue[i];

        // Increment the index of string also, so that it will skip the pattern part
        s_i += pattern_len;
    }

    // If pattern didn't exist then copy the original string to result
    if (res_i == 0)
    {
        strcpy(res, s);
    }
    else
    {
        // Copy left over string s (if any)
        while(res[res_i++] = s[s_i++])
            ; // Body of the loop

        // Mark the end of the string
        res[res_i] = '\0';
    }

    return res;
}

/**
 * Converts all upper alphabets to lower alphabets
 * usage:
 *    char s[100] = "THIS IS a test #1.";
 *    str_tolower(s);  OR printf("%s\n", str_tolower(s));
 *
 *    output:
 *      this is a test #1.
 *
 * bug:
 *    char *s = "this type of initialisation gives segfault";
 *    because this initialisation creates read only memory
 *
 *
 * @param char*  pointer to string
 * @return char*  original string base address (but string is in lower case)
 */
char *str_tolower(char *s)
{
	if (s == NULL) return s;

	for(char *p = s; *p; p++)
		*p = (unsigned char)tolower(*p);

	return s;
}

/**
 * Converts all lower alphabets to upper alphabets
 * usage:
 *    char s[100] = "THIS IS a test #1.";
 *    str_tolower(s);  OR printf("%s\n", str_tolower(s));
 *
 *    output:
 *      THIS IS A TEST #1.
 *
 * bug:
 *    char *s = "this type of initialisation gives segfault";
 *    because this initialisation creates read only memory
 *
 *
 * @param char*  pointer to string
 * @return char*  original string base address (but string is in upper case)
 */
char *str_toupper(char *s)
{
	if (s == NULL) return s;

	for(char *p = s; *p; p++)
		*p = (unsigned char)toupper(*p);

	return s;
}

int	str_split(char *in, char *delim, char **out, size_t ele)
{
	int i, nparams;
	struct Segment seg = {0};
	char *p = in;
	ssize_t sele = ele;

	nparams = 0;

	for (i = 0; i < sele - 1; i++) {
		if (!str_find_next(in, delim, &seg)) {
			out[i++] = p;
			nparams++;
			break;
		}

		out[i] = p;
		*((char *)seg.start_off) = 0;
		p = ((char *)seg.end_off) + 1;
		nparams++;
	}

	out[i] = NULL;

	return nparams;
}

