/*  The MIT License (MIT)

	Copyright (c) 2013 tohin, belko

	Permission is hereby granted, free of charge, to any person obtaining a copy of
	this software and associated documentation files (the "Software"), to deal in
	the Software without restriction, including without limitation the rights to
	use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
	the Software, and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
	COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
	IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.    */

#pragma once

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef LOG_PATH
#define LOG_PATH "."
#endif

extern "C" void __real_free(void* ptr);
extern "C" void* __real_malloc(size_t size);

static const int ONE_MEGABYTE = 1048576;
static const int TWENTY_FIVE_MEGABYTE = 26214400;

class CoreModule
{
public:

	static CoreModule& Instance()
	{
		static CoreModule instance;
		return instance;
	}

	void OnAllocation(void** buffer, int nStacks, size_t size, void* ptr)
	{
		int packet_size = 16 + (nStacks * 4);

		char* data = (char*) __real_malloc(packet_size + 4);

		write_int(&data[0], packet_size);
		write_int(&data[4], 1); // Packet Type
		write_int(&data[8], (int) ptr);
		write_int(&data[12], size);
		write_int(&data[16], nStacks);

		for (int i = 0; i < nStacks; ++i)
		{
			write_int(&data[20 + (4 * i)], reinterpret_cast<unsigned int>(buffer[i]));
		}

		fwrite(data, 1, packet_size + 4, output_file);
		__real_free(data);

		bytes_written += packet_size + 4;

		check_file_size_limit();
	}

	void OnDeAllocation(void** buffer, int nStacks, void* ptr)
	{
		int packet_size = 12 + (nStacks * 4);

		char* data = (char*) __real_malloc(packet_size + 4);

		write_int(&data[0], packet_size);
		write_int(&data[4], 2); // Packet Type
		write_int(&data[8], (int) ptr);
		write_int(&data[12], nStacks);

		for (int i = 0; i < nStacks; ++i)
		{
			write_int(&data[16 + (4 * i)], reinterpret_cast<unsigned int>(buffer[i]));
		}

		fwrite(data, 1, packet_size + 4, output_file);
		__real_free(data);

		bytes_written += packet_size + 4;

		check_file_size_limit();
	}

	void DumpInNextFile()
	{
		fclose(output_file);

		bytes_written = 0;
		file_count++;

		char file_path[64] = {0};

		sprintf(file_path, LOG_PATH"/memory_%d.raw", file_count);

		output_file = fopen(file_path, "wb");

		if (output_file == NULL)
		{
			printf("CANNOT OPEN FILE %s\n",file_path);
			exit(0);
		}

		setvbuf(output_file, file_buffer, _IOFBF, ONE_MEGABYTE);
	}

	void GetLock()
	{
		pthread_mutex_lock(&mutex);
	}

	void UnLock()
	{
		pthread_mutex_unlock(&mutex);
	}

private:

	CoreModule():
		file_count(0),
		bytes_written(0)
	{
		pthread_mutex_init(&mutex, NULL);

		output_file = fopen(LOG_PATH"/memory_0.raw", "wb");

		if (output_file == NULL)
		{
			printf("CANNOT OPEN FILE: " LOG_PATH"/memory_0.raw\n");
			exit(0);
		}

		setvbuf(output_file, file_buffer, _IOFBF, ONE_MEGABYTE);
	}

	CoreModule(const CoreModule& other) {}

	~CoreModule()
	{
		fclose(output_file);
	}


	void write_int(char* array, int n)
	{
		array[0] = (n >> 24) & 0xFF;
		array[1] = (n >> 16) & 0xFF;
		array[2] = (n >> 8) & 0xFF;
		array[3] = n & 0xFF;
	}

	void check_file_size_limit()
	{
		if (bytes_written >= TWENTY_FIVE_MEGABYTE)
		{
			DumpInNextFile();
		}
	}


	pthread_mutex_t mutex;

	char file_buffer[ONE_MEGABYTE];
	FILE* output_file;

	unsigned int file_count;
	unsigned int bytes_written;
};
