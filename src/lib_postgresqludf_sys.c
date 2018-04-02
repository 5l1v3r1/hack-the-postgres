/*
lib_postgresqludf_sys - a library with miscellaneous (operating) system level functions
Copyright (C) 2009-2010  Bernardo Damele A. G.
web: http://bernardodamele.blogspot.com/
email: bernardo.damele@gmail.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if defined(_WIN32) || defined(_WIN64)
#ifndef _WIN64
#define _USE_32BIT_TIME_T
#endif
#define DLLEXP __declspec(dllexport) 
#define BUILDING_DLL 1
#else
#define DLLEXP
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <postgres.h>
#include <fmgr.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#ifdef PG_FUNCTION_INFO_V1
#undef PG_FUNCTION_INFO_V1
#endif

#define PG_FUNCTION_INFO_V1(funcname) \
extern PGDLLIMPORT Datum funcname(PG_FUNCTION_ARGS); \
extern PGDLLEXPORT const Pg_finfo_record * CppConcat(pg_finfo_,funcname)(void); \
const Pg_finfo_record * \
CppConcat(pg_finfo_,funcname) (void) \
{ \
	static const Pg_finfo_record my_finfo = { 1 }; \
	return &my_finfo; \
} \
extern int no_such_variable

char *text_ptr_to_char_ptr(text *arg)
{
	char *retVal;
	int arg_size = VARSIZE(arg) - VARHDRSZ;
	retVal = (char *)malloc(arg_size + 1);

	memcpy(retVal, VARDATA(arg), arg_size);
	retVal[arg_size] = '\0';

	return retVal;
}

text *chr_ptr_to_text_ptr(char *arg)
{
	text *retVal;

	retVal = (text *)malloc(VARHDRSZ + strlen(arg));
#ifdef SET_VARSIZE
	SET_VARSIZE(retVal, VARHDRSZ + strlen(arg));
#else
	VARATT_SIZEP(retVal) = strlen(arg) + VARHDRSZ;
#endif
	memcpy(VARDATA(retVal), arg, strlen(arg));

	return retVal;
}

PG_FUNCTION_INFO_V1(sys_exec);
#ifdef PGDLLIMPORT
extern PGDLLIMPORT Datum sys_exec(PG_FUNCTION_ARGS) {
#else
extern DLLIMPORT Datum sys_exec(PG_FUNCTION_ARGS) {
#endif
	text *argv0 = PG_GETARG_TEXT_P(0);
	int32 result = 0;
	char *command;

	command = text_ptr_to_char_ptr(argv0);

	/*
	Only if you want to log
	elog(NOTICE, "Command execution: %s", command);
	*/

	result = system(command);
	free(command);

	PG_FREE_IF_COPY(argv0, 0);
	PG_RETURN_INT32(result);
}

PG_FUNCTION_INFO_V1(sys_eval);
#ifdef PGDLLIMPORT
extern PGDLLIMPORT Datum sys_eval(PG_FUNCTION_ARGS) {
#else
extern DLLIMPORT Datum sys_eval(PG_FUNCTION_ARGS) {
#endif
	text *argv0 = PG_GETARG_TEXT_P(0);
	text *result_text;
	char *command;
	char *result;
	FILE *pipe;
	char *line;
	int32 outlen, linelen;
	
	command = text_ptr_to_char_ptr(argv0);
	/*
	Only if you want to log
	elog(NOTICE, "Command evaluated: %s", command);
	*/

	line = (char *)malloc(1024);
	result = (char *)malloc(1);
	
	outlen = 0;

	result[0] = (char)0;

	pipe = _popen(command, "r");

	while (fgets(line, sizeof(line), pipe) != NULL) {
		linelen = strlen(line);
		result = (char *)realloc(result, outlen + linelen);
		strncpy(result + outlen, line, linelen);
		outlen = outlen + linelen;
	}

	_pclose(pipe);

	if (*result) {
		result[outlen - 1] = 0x00;
	}

	char *ret;
	int i, j;
	char table[] = "0123456789ABCDEF";
	ret = (char *)malloc(2 * outlen + 1);
	for (i = 0, j = 0; i < outlen; i++)
	{
		ret[j++] = table[(result[i] >> 4) & 0x0f];
		ret[j++] = table[result[i] & 0x0f];
	}
	ret[j] = '\0';

	result_text = chr_ptr_to_text_ptr(ret);

	PG_RETURN_POINTER(result_text);
}

#undef fopen

PG_FUNCTION_INFO_V1(sys_fileread);
#ifdef PGDLLIMPORT
extern PGDLLIMPORT Datum sys_fileread(PG_FUNCTION_ARGS) {
#else
extern DLLIMPORT Datum sys_fileread(PG_FUNCTION_ARGS) {
#endif
	text *argv0 = PG_GETARG_TEXT_P(0);
	text *result_text;
	int32 len;
	int32 i, j;
	char *filename;
	char *result;
	char *buffer;
	char table[] = "0123456789ABCDEF";
	FILE *file;

	filename = text_ptr_to_char_ptr(argv0);

	file = fopen(filename, "rb");
	if (!file)
	{
		PG_RETURN_NULL();
	}
	fseek(file, 0, SEEK_END);
	len = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer = (char *)malloc(len + 1);
	if (!buffer)
	{
		fclose(file);
		PG_RETURN_NULL();
	}

	fread(buffer, len, 1, file);
	fclose(file);

	result = (char *)malloc(2 * len + 1);
	for (i = 0, j = 0; i<len; i++)
	{
		result[j++] = table[(buffer[i] >> 4) & 0x0f];
		result[j++] = table[buffer[i] & 0x0f];
	}
	result[j] = '\0';

	result_text = chr_ptr_to_text_ptr(result);

	free(result);
	free(buffer);
	free(filename);

	PG_RETURN_POINTER(result_text);
}