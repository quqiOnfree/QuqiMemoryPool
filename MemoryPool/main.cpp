//test file
//测试文件

#include <iostream>
#include <ctime>
#include "MemoryPool.h"

using namespace std;

int main()
{
	qmem::SingleDataTypeMemoryPool<size_t> sd;
	size_t times = 1000000000;
	size_t* a;

	clock_t start = clock();
	for (size_t i = 0; i < times; ++i)
	{
		a = sd.allocate();
		sd.deallocate(a);
	}
	cout << static_cast<long double>(clock() - start) / CLOCKS_PER_SEC / times << '\n';

	qmem::BaseMemoryPool bmp;

	start = clock();
	for (size_t i = 0; i < times; ++i)
	{
		a = bmp.allocate<size_t>();
		bmp.deallocate(a);
	}
	cout << static_cast<long double>(clock() - start) / CLOCKS_PER_SEC / times << '\n';

	return 0;
}