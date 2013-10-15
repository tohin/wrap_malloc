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

#include <new>
#include <stdlib.h>
#include <stdio.h>

#include "stack.h"
#include "CoreModule.h"

static unsigned long int TWO_GB = 2147483648u;

////////////////////////////////////////////////////////////////////////////////
//                                 malloc                                     //
////////////////////////////////////////////////////////////////////////////////

extern "C" void* __real_malloc(size_t size);

extern "C" void* __wrap_malloc(size_t size)
{
	void* buffer[MAX_STACK_SIZE * 4] = {0};
	int nStacks = 0;
	void* ptr = NULL;
	
	CoreModule::Instance().GetLock();

	if (size == TWO_GB) // 2Gb  ^_^
	{
		CoreModule::Instance().DumpInNextFile();
		ptr = NULL;
	}
	else
	{
		ptr = __real_malloc(size);

		nStacks = get_current_stack(buffer);

		CoreModule::Instance().OnAllocation(buffer, nStacks, size, ptr);
	}

	CoreModule::Instance().UnLock();

	return ptr;
}

////////////////////////////////////////////////////////////////////////////////
//                                 calloc                                     //
////////////////////////////////////////////////////////////////////////////////

extern "C" void* __real_calloc(size_t num, size_t size);

extern "C" void* __wrap_calloc(size_t num, size_t size)
{
	void* buffer[MAX_STACK_SIZE * 4] = {0};
	int nStacks = 0;
	void* ptr = NULL;

	CoreModule::Instance().GetLock();

	ptr = __real_calloc(num, size);

    nStacks = get_current_stack(buffer);

    CoreModule::Instance().OnAllocation(buffer, nStacks, (size * num), ptr);

	CoreModule::Instance().UnLock();

	return ptr;
}

////////////////////////////////////////////////////////////////////////////////
//                                 realloc                                    //
////////////////////////////////////////////////////////////////////////////////

extern "C" void* __real_realloc(void* ptr, size_t size);

extern "C" void* __wrap_realloc(void* ptr, size_t size)
{
	void* buffer[MAX_STACK_SIZE * 4] = {0};
	int nStacks = 0;
	void* new_ptr = NULL;

	CoreModule::Instance().GetLock();

	new_ptr = __real_realloc(ptr, size);

	nStacks = get_current_stack(buffer);

	CoreModule::Instance().OnDeAllocation(buffer, nStacks, ptr);

	CoreModule::Instance().OnAllocation(buffer, nStacks, size, new_ptr);

	CoreModule::Instance().UnLock();

	return new_ptr;
}

////////////////////////////////////////////////////////////////////////////////
//                                  free                                      //
////////////////////////////////////////////////////////////////////////////////

extern "C" void __real_free(void* ptr);

extern "C" void __wrap_free(void* ptr)
{
	// 4 is the size of a pointer
	void* buffer[MAX_STACK_SIZE * 4] = {0};
	int nStacks = 0;

	CoreModule::Instance().GetLock();

    __real_free(ptr);

    nStacks = get_current_stack(buffer);

    CoreModule::Instance().OnDeAllocation(buffer, nStacks, ptr);

    CoreModule::Instance().UnLock();
}

////////////////////////////////////////////////////////////////////////////////
//                              C++ wrappers                                  //
////////////////////////////////////////////////////////////////////////////////

void* operator new(size_t nSize) throw (std::bad_alloc)
{
	return malloc(nSize);
}

void* operator new[] (size_t nSize) throw (std::bad_alloc)
{
	return malloc(nSize);
}

void operator delete (void* ptr) throw()
{
	return free(ptr);
}

void operator delete[] (void* ptr) throw()
{
	return free(ptr);
}
