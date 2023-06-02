# 内存池MemoryPool

- 模板类型为class时 使用格式如下
```cpp

struct hi
{
	hi() {}
	vector<int> a;
	~hi() {}
};
qmem::SingleDataTypeMemoryPool<hi> sd2;
//allocate 获取内存
//自动调用构造函数
hi* qw = sd2.allocate();
//归还内存
sd2.deallocate(qw);

```

- 不同内存池之间的性能
```cpp

{
	//模板类型为class时 使用格式如下
	qmem::SingleDataTypeMemoryPool<hi> sd2;
	//allocate 获取内存
	//自动调用构造函数
	hi* qw = sd2.allocate();
	//归还内存
	sd2.deallocate(qw);
}

{
	//允许单个类型获取内存的内存池，不允许生成数组
	//release 获取内存和释放内存平均耗时 1.597e-08s
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
	cout << "SingleDataTypeMemoryPool：" << static_cast<long double>(clock() - start) / CLOCKS_PER_SEC / times << '\n';
}

{
	//标准库内存池
	//release 获取内存和释放内存平均耗时 9.122e-08s
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
	cout << "allocator：" << static_cast<long double>(clock() - start) / CLOCKS_PER_SEC / times << '\n';
}

```
