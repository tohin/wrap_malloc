#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <pthread.h> 

class B
{
public:
	char b;
};

class A
{
public:
	B* b;

	A()
	{
		b = new B;
	}

	void fn1()
	{
		fn2();
	}

	void fn2()
	{
		fn3();
	}

	void fn3()
	{
		delete b;
		b = new B;
	}

	~A()
	{
		delete b;
	}
};


int main()
{
	int* q = new int;
	delete q;

	A* a = new A;
	a->fn1();
	delete a;

	//malloc(2147483648u);

	void* thread_func(void*);
	pthread_t thread1;
	pthread_create(&thread1, NULL, thread_func, NULL);
	pthread_join(thread1, NULL);

	malloc(2147483648u);

	pthread_create(&thread1, NULL, thread_func, NULL);
	pthread_join(thread1, NULL);

	malloc(2147483648u);

	pthread_create(&thread1, NULL, thread_func, NULL);
	pthread_join(thread1, NULL);

	printf("exit\n");

	return 0;
}

void fn1()
{
	int* q = new int;
}

void* thread_func(void*)
{
	A* a = new A;
	delete a;

	for (int i = 0; i < 3; ++i)
	{
		fn1();
	}
}