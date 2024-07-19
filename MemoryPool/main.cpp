/*
   Copyright 2023-2024 Xuan Xiao

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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
	{
		//安全测试

		qmem::SingleDataTypeMemoryPool<size_t> sd;
		for (size_t i = 0; i < 4096; i++)
		{
			a[i] = sd.allocate();
			*a[i] = i;
		}
		bool correct = true;
		for (size_t i = 0; i < 4096; i++)
		{
			cout << *a[i] << ' ';
			if (*a[i] != i)
			{
				correct = false;
				break;
			}
		}
		cout << '\n';
		if (correct)
		{
			cout << "数据无误\n";
		}
		else
		{
			cout << "数据有误\n";
		}
		for (size_t i = 0; i < 4096; i++)
		{
			sd.deallocate(a[i]);
		}
	}

	{
		//性能测试

		long double sdtm = 0, sa = 0;

		for (size_t i = 0; i < 5; i++)
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
			if (i == 0)
			{
				sdtm = end;
			}
			else
			{
				sdtm = (end + sdtm) / 2;
			}
			cout <<"times: "<< i << " SingleDataTypeMemoryPool: " << end << '\n';
		}

		for (size_t i = 0; i < 5; i++)
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
			if (i == 0)
			{
				sa = end;
			}
			else
			{
				sa = (end + sa) / 2;
			}
			cout << "times: " << i << " allocator: " << end << '\n';
		}

		cout << "自制内存池快于标准库内存池" << sa / sdtm << "倍\n";
	}

	return 0;
}
