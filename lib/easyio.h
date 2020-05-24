#ifndef EASYIO_H
#define EASYIO_H

typedef char * String;

unsigned int GetString(char **, FILE *);
int GetInt(void);
double GetDouble(void);


/**
 * GetString(FILE *stream): reads string from stdin and intelligently
 * stores it in annonymos memory and finally returns the
 * char* to calling function.
 *
 * It doesn't include trailing newline character. String
 * will be stored in memory this way:
 * +---+---+---+---+---+----+
 * | H | E | L | L | O | \0 |
 * +---+---+---+---+---+----+
 *
 * Usage: String s = GetString(stdin)
 * Usage: String s = GetString(FILE *fp)
 */
unsigned int GetString(char **line, FILE *fp)
{
	unsigned int size = 100, c, written;
	char *s, *news;

	s = (String)malloc(sizeof(char) * size);
	written = 0;
	while((c = getc(fp)) != '\n' && c != EOF)
	{
		/* written + 2 because, for null byte space and indext starts from 0 */
		if(written + 2 >= size)
		{
			news = (char *)realloc(s, size += size);

			/* recovering on failure */
			if(news)
				s = news; // s gets address of newly allocated mem
			else
				s = s;    // s gets old address, recovering from failure
		}
		s[written++] = c;
	}

	s[written] = '\0';
	*line = s;

	return written;
}

/**
 * GetInt() function returns first occurence of int
 * from the input stream stdin.
 * This function solves the issue of scanf(), when
 * integer is expecting (%d) and non number is entered
 */
int GetInt(void)
{
	int num;
	char *s;
	GetString(&s, stdin);
	sscanf(s, "%d", &num);
	return num;
}

/**
 * GetDouble() function returns first occurence of double
 * from the input stream stdin.
 * This function solves the issue of scanf(), when
 * double is expecting (%f) and non number is entered
 */
double GetDouble(void)
{
	double num;
	char *s;
	GetString(&s, stdin);
	sscanf(s, "%lf", &num);
	return num;
}



#endif
