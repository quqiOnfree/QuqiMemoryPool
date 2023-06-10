//test file
//测试文件

#include <iostream>
#include <ctime>
#include <memory>
#include <vector>
#include <thread>
#include <random>
#include <algorithm>
#include <chrono>
#include <stack>
#include <functional>

#include "MemoryPool.hpp"

using namespace std;

template<size_t blockSize = 64>
struct Data
{
	size_t block[blockSize]{ 0 };
};

template<
	typename T = Data<64>,
	typename Alloc = std::allocator<T>,
	long long SUM = 5000,
	long long MEDIUM = 10240,
	long long OFFSET = 5120,
	long long MIN = 1024,
	long long MAX = 20480
>
class MemoryPoolTest
{
public:
	MemoryPoolTest(Alloc& alloc) :
		m_nd(MEDIUM, OFFSET),
		m_stack(MAX),
		m_mt(std::random_device()()),
		m_memoryPool(alloc)
	{
		for (size_t i = 0; i <= SUM; i++)
		{
			long long x = (long long)std::round(m_nd(m_mt));
			x = std::max(MIN, x);
			x = std::min(MAX, x);
			m_num.push_back(x);
		}

		for (size_t i = m_num.size() - 1; i >= 1; i--)
		{
			m_num[i] -= m_num[i - 1];
		}
	}

	template<typename AllocFunc, typename DeallocFunc>
	long double allocTest(AllocFunc allocFunc, DeallocFunc deAllocFunc)
	{
		std::shuffle(m_stack.begin(), m_stack.end(), m_mt);
		auto alloc = std::bind(allocFunc, m_memoryPool, 1);

		auto start = std::clock();
		for (auto i : m_num)
		{
			if (i >= 0)
			{
				for (long long j = 0; j < i; j++)
				{
					m_stack[top++] = alloc();
				}
				
			}
			else
			{
				for (long long j = 0; j > i; j--)
				{
					std::bind(deAllocFunc, m_memoryPool, m_stack[--top], 1)();
				}
			}
		}

		while (top)
		{
			std::bind(deAllocFunc, m_memoryPool, m_stack[--top], 1)();
		}

		auto end = std::clock();

		return (end - start) * 1.0l / CLOCKS_PER_SEC;
	}
	
private:
	Alloc m_memoryPool;
	std::mt19937_64 m_mt;
	std::normal_distribution<> m_nd;
	std::vector<long long> m_num;
	std::vector<T*> m_stack;
	size_t top = 0;
};

constexpr size_t times = 10000000;
size_t* a[times]{ 0 };

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

	/*{
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
	}*/

	//自己写的
	for (int i = 0; i < 5; i++)
	{
		qmem::SingleDataTypeMemoryPool<Data<64>> alloc;
		MemoryPoolTest<Data<64>, qmem::SingleDataTypeMemoryPool<Data<64>>> mpt(alloc);
		auto get = mpt.allocTest(&qmem::SingleDataTypeMemoryPool<Data<64>>::allocate, &qmem::SingleDataTypeMemoryPool<Data<64>>::deallocate);
		cout << "qmem::SingleDataTypeMemoryPool: " << get << '\n';
	}

	//标准库
	for (int i = 0; i < 5; i++)
	{
		std::allocator<Data<64>> alloc;
		MemoryPoolTest<> mpt(alloc);
		auto get = mpt.allocTest(&std::allocator<Data<64>>::allocate, &std::allocator<Data<64>>::deallocate);
		cout << "std::allocator: " << get << '\n';
	}

	

	return 0;
}
