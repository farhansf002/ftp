#include <ftp.h>
#include <dirent.h>

int ftp_command_find(struct ftp *req)
{
	for (int i = 0; commands[i].cmd; ++i) {
		if (strcmp(commands[i].cmd, req->cmd) == 0)
			return i;
	}

	return -1;
}

int ftp_is_file(const char *filepath)
{
	struct stat statbuf;
	
	if (stat(filepath, &statbuf) == -1) {
		return -1;
	}

	return (statbuf.st_mode & S_IFMT) == S_IFREG;
}

int is_dir(const char *filepath)
{
	DIR *dp;
	struct dirent *dentp;

	if ((dp = opendir(filepath)) == NULL) {
		closedir(dp);
		return 0;
	}

	else 
		return 1;
}

int ftp_dir_list_get(const char *filepath, char **res)
{
	DIR *dp;
	struct dirent *dentp;
	char *buf = calloc(1, BUFSIZ);
	int l = 0;

	*res = NULL;

	if (buf == NULL)
		return l;

	if ((dp = opendir(filepath)) == NULL) {
		closedir(dp);
		return l;
	}

	while ((dentp = readdir(dp)) != NULL) {
		/* Skip hidden or special files */
		if (dentp->d_name[0] == '.')
			continue;
		strcat(buf, dentp->d_name);
		l = strlen(buf);
		buf[l++] = '\n';
		buf[l] = 0;
	}

	*res = buf;

	return l + 1;
}

int ftp_file_details_get(const char *filepath, char **res)
{
	struct stat statbuf;
	char *buf = calloc(1, 200), tmp[50];
	int l = 0;

	*res = NULL;

	if (buf == NULL)
		return l;

	if (stat(filepath, &statbuf) == -1)
		return -1;

	strcat(buf, filepath);
	l = strlen(buf);
	buf[l++] = '\t';
	char unit[3];
	double s = show_unit(statbuf.st_size, unit);
	sprintf(tmp, "%.2lf %s", s, unit);
	strcat(buf, tmp);
	l = strlen(buf);

	*res = buf;

	return l;
}
double show_unit(size_t size, char *unit)
{
	double s = size;
	unit[0] = 'B';
	unit[1] = unit[2] = 0;
	if(s / 1000 >= 1)
	{
		s = s/1024;
		unit[0] = 'K';
		unit[1] = 'B';
	}
	if(s / 1000 >= 1)
	{
		s = s / 1024;
		unit[0] = 'M';
	}
	if(s / 1000 >= 1)
	{
		s = s/1024;
		unit[0] = 'G';
	}
	return s;
}
