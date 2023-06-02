//test file
//测试文件

#include <iostream>
#include <ctime>
#include <memory>
#include <vector>
#include <thread>
#include "MemoryPool.hpp"

using namespace std;

struct test
{
	size_t a[10];
};

const size_t times = 100000000;
size_t* a[times];

int main()
{
	long double sdtm = 0, sa = 0;

	{
		//允许单个类型获取内存的内存池，不允许生成数组
		qmem::SingleDataTypeMemoryPool<size_t> sd;
		clock_t start = clock();
		for (size_t i = 0; i < times; ++i)
		{
			a[i] = sd.allocate();
		}
		for (size_t i = 0; i < times; ++i)
		{
			sd.deallocate(a[i]);
		}
		long double end = static_cast<long double>(clock() - start) / CLOCKS_PER_SEC / times;
		sdtm = end;
		cout << "SingleDataTypeMemoryPool：" << end << '\n';
	}

	{
		//标准库内存池
		allocator<size_t> stdAllocator;
		clock_t start = clock();
		for (size_t i = 0; i < times; ++i)
		{
			a[i] = stdAllocator.allocate(1);
		}
		for (size_t i = 0; i < times; ++i)
		{
			stdAllocator.deallocate(a[i], 1);
		}
		long double end = static_cast<long double>(clock() - start) / CLOCKS_PER_SEC / times;
		sa = end;
		cout << "allocator：" << end << '\n';
	}

	cout << "自制内存池快于标准库内存池" << sa / sdtm << "倍\n";

	return 0;
}
