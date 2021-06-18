#define _CRT_SECURE_NO_WARNINGS
#include "funcs.h"
#define SIZE 1000

char** ReadData(const char* input_name, int* rows)
{
	FILE* in = fopen(input_name, "r");

	char** data = (char**)calloc(SIZE, sizeof(char*));
	for (int i = 0; i < SIZE; i++)
	{
		data[i] = (char*)calloc(SIZE, sizeof(char));
	}

	*rows = 0;
	while (!feof(in))
	{
		fgets(data[*rows], SIZE, in);
		(*rows)++;
	}

	fclose(in);
	return data;
}

void AddData(char** data, const char* str, int* rows)
{
	strcpy(data[*rows], str);
	(*rows)++;
}

void WriteData(const char* output_name, int rows, char** data)
{
	FILE* out = fopen(output_name, "w");

	for (int i = 0; i < rows; i++)
		fprintf(out, data[i]);

	fclose(out);
}

char* ReadHistory(const char* name)
{
	FILE* in = fopen(name, "a+");

	char* history = (char*)calloc(1024, sizeof(char));
	int bytes = fread(history, sizeof(char), 1024, in);

	history[bytes] = 0;

	if (bytes > 0)
		strcat(history, "-------------\n");

	return history;
}
