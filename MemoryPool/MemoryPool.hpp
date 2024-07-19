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

#pragma once

#include <cstring>

namespace qmem
{
	class BaseMemoryPool
	{
	public:
		BaseMemoryPool()
			:BaseMemoryPool(4096)
		{

		}
		BaseMemoryPool(size_t blockSize)
			:p_blockSize(blockSize)
		{

		}
		BaseMemoryPool(const BaseMemoryPool&) = delete;
		BaseMemoryPool(BaseMemoryPool&& bmp) noexcept
		{
			p_blockSize = bmp.p_blockSize;
			p_startElementNode = bmp.p_startElementNode;
			bmp.p_startElementNode = nullptr;
		}
		~BaseMemoryPool()
		{
			while (p_startElementNode != nullptr)
			{
				delete p_startElementNode->sB;
				ElementNode* temp = p_startElementNode;
				p_startElementNode = p_startElementNode->next;
				delete temp;
			}
		}

		//允许申请数组
		template<typename T>
		T* allocate(size_t length = 1)
		{
			size_t requestSize = sizeof(T) * length;
			if (p_startElementNode == nullptr)
			{
				p_startElementNode = new ElementNode{ 1,new SingleBlock(p_blockSize),nullptr,nullptr };
			}
			if (requestSize > p_blockSize / 2)
			{
				ElementNode* indexElementNode = p_startElementNode;
				while (indexElementNode->next != nullptr)
				{
					indexElementNode = indexElementNode->next;
				}
				indexElementNode->next = new ElementNode{ 2,nullptr,new char[requestSize] {0} };
				return reinterpret_cast<T*>(indexElementNode->next->largeElement);
			}
			ElementNode* indexElementNode = p_startElementNode;
			ElementNode* beforeElementNode = nullptr;
			while (indexElementNode != nullptr)
			{
				void* getedPointer = indexElementNode->sB->get(requestSize);
				if (indexElementNode->mode == 1 && getedPointer != nullptr)
					return reinterpret_cast<T*>(getedPointer);
				beforeElementNode = indexElementNode;
				indexElementNode = indexElementNode->next;
			}
			beforeElementNode->next = new ElementNode{ 1,new SingleBlock(p_blockSize),nullptr,nullptr };

			T* getValue = reinterpret_cast<T*>(beforeElementNode->next->sB->get(requestSize));
			if (std::is_class<T>::value)
			{
				return new(getValue) T;
			}
			return getValue;
		}

		template<typename T>
		void deallocate(T* pointer)
		{
			ElementNode* indexElementNode = p_startElementNode;
			ElementNode* beforeElementNode = nullptr;
			while (indexElementNode != nullptr)
			{
				if (indexElementNode->mode == 1 && !indexElementNode->sB->postBack(pointer))
					return;

				else if (indexElementNode->mode == 2 && indexElementNode->largeElement == pointer)
				{
					delete[] pointer;
					return;
				}

				indexElementNode = indexElementNode->next;
			}
		}

		BaseMemoryPool& operator=(const BaseMemoryPool&) = delete;
		BaseMemoryPool& operator=(BaseMemoryPool&& bmp) noexcept
		{
			if (this != &bmp)
			{
				this->~BaseMemoryPool();

				p_blockSize = bmp.p_blockSize;
				p_startElementNode = bmp.p_startElementNode;
				bmp.p_startElementNode = nullptr;
			}

			return *this;
		}
	protected:
		class SingleBlock;

		struct ElementNode
		{
			int mode = 0;
			SingleBlock* sB;
			void* largeElement = nullptr;
			ElementNode* next = nullptr;
		};

		class SingleBlock
		{
		public:
			SingleBlock(size_t blockSize = 4096)
				:p_blockPointerSize(blockSize)
			{
				p_blockPointer = reinterpret_cast<void*>(new char[p_blockPointerSize + sizeof(PointerNode) * p_blockPointerSize] {0});
				p_startPointerNode = reinterpret_cast<PointerNode*>(reinterpret_cast<char*>(p_blockPointer) + p_blockPointerSize);
				p_indexPointerNode = p_startPointerNode;
				p_lastPointerNode = p_startPointerNode + p_blockPointerSize;
				p_indexPointerNode->valuePointer = p_blockPointer;
				p_indexPointerNode->localBlockSize = p_blockPointerSize;
			}
			~SingleBlock()
			{
				delete[] reinterpret_cast<char*>(p_blockPointer);
			}

			void* get(size_t size)
			{
				if (p_freePointerNode != nullptr)
				{
					PointerNode* indexPointerNode = p_freePointerNode;
					while (indexPointerNode != nullptr)
					{
						if (indexPointerNode->localBlockSize == size)
						{
							p_freePointerNode = indexPointerNode->next;
							indexPointerNode->next = nullptr;
							return reinterpret_cast<void*>(indexPointerNode);
						}
						else if (indexPointerNode->localBlockSize > size)
						{
							PointerNode* newPointerNode = nullptr;
							if (p_reusefulPointerNode != nullptr)
							{
								newPointerNode = p_reusefulPointerNode;
								p_reusefulPointerNode = p_reusefulPointerNode->next;
							}
							else if (p_indexPointerNode != p_lastPointerNode)
							{
								newPointerNode = p_indexPointerNode++;
								::memcpy_s(p_indexPointerNode, sizeof(PointerNode), newPointerNode, sizeof(PointerNode));
							}
							else
							{
								return nullptr;
							}
							newPointerNode->localBlockSize = indexPointerNode->localBlockSize - size;
							newPointerNode->valuePointer = reinterpret_cast<char*>(indexPointerNode->valuePointer) + size;
							indexPointerNode->localBlockSize = size;
							newPointerNode->next = p_freePointerNode->next;
							p_freePointerNode = newPointerNode;
							indexPointerNode->next = nullptr;
							return reinterpret_cast<void*>(indexPointerNode);
						}
						indexPointerNode = indexPointerNode->next;
					}
				}
				if (size > p_indexPointerNode->localBlockSize)
					return nullptr;
				if (p_indexPointerNode != p_lastPointerNode)
				{
					PointerNode* indexPointerNode = p_indexPointerNode;
					PointerNode* nextPointerNode = ++p_indexPointerNode;
					nextPointerNode->localBlockSize = indexPointerNode->localBlockSize - size;
					indexPointerNode->localBlockSize = size;
					nextPointerNode->valuePointer = reinterpret_cast<char*>(indexPointerNode->valuePointer) + size;
					return reinterpret_cast<void*>(indexPointerNode);
				}
				else
				{
					return nullptr;
				}
			}

			int postBack(void* back)
			{
				PointerNode* indexPointerNode = reinterpret_cast<PointerNode*>(back);

				if (p_freePointerNode == nullptr)
				{
					indexPointerNode->next = nullptr;
					p_freePointerNode = indexPointerNode;
				}
				else
				{
					indexPointerNode->next = p_freePointerNode;
					p_freePointerNode = indexPointerNode;
				}
				//排序
				return 0;
			}

		protected:
			struct PointerNode
			{
				void* valuePointer = nullptr;
				size_t localBlockSize = 0;
				PointerNode* next = nullptr;
			};
		private:
			PointerNode* p_startPointerNode = nullptr;
			PointerNode* p_indexPointerNode = nullptr;
			PointerNode* p_lastPointerNode = nullptr;

			PointerNode* p_reusefulPointerNode = nullptr;
			PointerNode* p_freePointerNode = nullptr;

			void* p_blockPointer = nullptr;
			size_t p_blockPointerSize;
		};
	private:
		size_t p_blockSize;
		ElementNode* p_startElementNode = nullptr;
	};

	template<typename T>
	class SingleDataTypeMemoryPool
	{
	public:
		//内存释放检查模式
		enum class CheckMode
		{
			//内存释放检查
			Check,
			//内存释放不检查
			NoCheck
		};

		SingleDataTypeMemoryPool()
			:SingleDataTypeMemoryPool(2048)
		{
		}

		SingleDataTypeMemoryPool(size_t dataTypeCount)
			:p_dataTypeCount(dataTypeCount), p_checkMode(CheckMode::Check)
		{
		}

		SingleDataTypeMemoryPool(const SingleDataTypeMemoryPool&) = delete;
		SingleDataTypeMemoryPool(SingleDataTypeMemoryPool&& sdtm) noexcept
		{
			p_dataTypeCount = sdtm.p_dataTypeCount;

			p_startElementNode = sdtm.p_startElementNode;
			sdtm.p_startElementNode = nullptr;

			p_startPointerNode = sdtm.p_startPointerNode;
			sdtm.p_startPointerNode = nullptr;
			p_indexPointerNode = sdtm.p_indexPointerNode;
			sdtm.p_indexPointerNode = nullptr;
			p_endPointerNode = sdtm.p_endPointerNode;
			sdtm.p_endPointerNode = nullptr;
			p_freePointerNode = sdtm.p_freePointerNode;
			sdtm.p_freePointerNode = nullptr;
		}

		~SingleDataTypeMemoryPool()
		{
			// 释放每个内存块

			ElementNode* localElementNode = nullptr;
			while (p_startElementNode != nullptr)
			{
				localElementNode = p_startElementNode;
				p_startElementNode = p_startElementNode->next;
				delete[] reinterpret_cast<char*>(localElementNode);
			}
		}

		SingleDataTypeMemoryPool& operator=(const SingleDataTypeMemoryPool&) = delete;
		SingleDataTypeMemoryPool& operator=(SingleDataTypeMemoryPool&& sdtm) noexcept
		{
			if (this == &sdtm)
				return *this;

			this->~SingleDataTypeMemoryPool();

			p_dataTypeCount = sdtm.p_dataTypeCount;

			p_startElementNode = sdtm.p_startElementNode;
			sdtm.p_startElementNode = nullptr;

			p_startPointerNode = sdtm.p_startPointerNode;
			sdtm.p_startPointerNode = nullptr;
			p_indexPointerNode = sdtm.p_indexPointerNode;
			sdtm.p_indexPointerNode = nullptr;
			p_endPointerNode = sdtm.p_endPointerNode;
			sdtm.p_endPointerNode = nullptr;
			p_freePointerNode = sdtm.p_freePointerNode;
			sdtm.p_freePointerNode = nullptr;
			
			return *this;
		}

		//设置内存释放检查模式
		void setCheckMode(CheckMode cm)
		{
			p_checkMode = cm;
		}

		// 申请内存
		// 不允许申请数组
		template<typename... Args>
		T* allocate(Args&&... args)
		{
			if (p_freePointerNode != nullptr)
			{
				//释放指针链表不为空，可以重复利用

				PointerNode* indexPointerNode = p_freePointerNode;
				p_freePointerNode = p_freePointerNode->next;
				indexPointerNode->next = nullptr;

				T* getValue = reinterpret_cast<T*>(indexPointerNode);
				if (std::is_class<T>::value)
				{
					new(getValue) T(std::forward<Args>(args)...);
				}
				return getValue;
			}

			if (p_indexPointerNode >= p_endPointerNode)
			{
				// 如果PointNode到了末尾
				// 意味着内存中所有的元素已经分配完成
				// 就是要申请新的内存块了
				allocateBlock();
			}

			// PointNode没有到末尾
			// 获取未使用的PointerNode
			
			// 新的的PointerNode
			PointerNode* indexPointerNode = p_indexPointerNode++;

			//PointerNode*转成元素指针
			T* getValue = reinterpret_cast<T*>(indexPointerNode);
			if (std::is_class<T>::value)
			{
				new(getValue) T(std::forward<Args>(args)...);
			}
			return getValue;
		}

		// 释放内存
		void deallocate(T* backPointer)
		{
			// 如果为 nullptr
			if (backPointer == nullptr) throw std::runtime_error("this pointer isn't in the memory pool");

			if (p_checkMode == CheckMode::Check)
			{
				bool hasFind = false;
				ElementNode* localElement = p_startElementNode;
				while (localElement != nullptr)
				{
					if (localElement->startMemoryBlock <= reinterpret_cast<char*>(backPointer) && reinterpret_cast<char*>(backPointer) < localElement->endMemoryBlock)
					{
						hasFind = true;
						break;
					}
					localElement = localElement->next;
				}
				if (!hasFind) throw std::runtime_error("this pointer isn't in the memory pool");
			}

			if (std::is_class<T>::value)
			{
				backPointer->~T();
			}

			//存到释放链表中
			PointerNode* index = reinterpret_cast<PointerNode*>(backPointer);
			index->next = p_freePointerNode;
			p_freePointerNode = index;
		}

	protected:
		// 申请内存块
		void allocateBlock()
		{
			if (p_startElementNode == nullptr)
			{
				// 如果一个内存块都没申请

				// 申请新的内存块（包括内存池的内存、元素结构体指针的内存）
				char* localPointer = new char[sizeof(ElementNode) + sizeof(PointerNode) * p_dataTypeCount] {0};

				//更新内存池链表起始位置
				p_startElementNode = reinterpret_cast<ElementNode*>(localPointer);
				p_startElementNode->startMemoryBlock = reinterpret_cast<char*>(localPointer + sizeof(ElementNode));
				p_startElementNode->endMemoryBlock = reinterpret_cast<char*>(localPointer + sizeof(ElementNode) + sizeof(PointerNode) * p_dataTypeCount);
				p_startElementNode->pointerNodePointer = reinterpret_cast<PointerNode*>(localPointer + sizeof(ElementNode));

				// 更新元素结构体链表起始位置
				p_startPointerNode = p_startElementNode->pointerNodePointer;
				p_indexPointerNode = p_startPointerNode;
				p_endPointerNode = p_startPointerNode + p_dataTypeCount;
			}
			else
			{
				// 如果已经有内存块

				ElementNode* localElementPointer = p_startElementNode;
				//扩容
				p_dataTypeCount *= 2;
				// 申请新的内存块（包括内存池的内存、元素结构体指针的内存）
				char* localPointer = new char[sizeof(ElementNode) + sizeof(PointerNode) * p_dataTypeCount] {0};

				// 更新内存池链表起始位置
				p_startElementNode = reinterpret_cast<ElementNode*>(localPointer);
				p_startElementNode->startMemoryBlock = reinterpret_cast<char*>(localPointer + sizeof(ElementNode));
				p_startElementNode->endMemoryBlock = reinterpret_cast<char*>(localPointer + sizeof(ElementNode) + sizeof(PointerNode) * p_dataTypeCount);
				p_startElementNode->pointerNodePointer = reinterpret_cast<PointerNode*>(localPointer + sizeof(ElementNode));
				p_startElementNode->next = localElementPointer;

				// 更新元素结构体链表起始位置
				p_startPointerNode = p_startElementNode->pointerNodePointer;
				p_indexPointerNode = p_startPointerNode;
				p_endPointerNode = p_startPointerNode + p_dataTypeCount;
			}
		}

		struct PointerNode;

		// 内存池指针
		struct ElementNode
		{
			//内存块起始位置
			char* startMemoryBlock = nullptr;
			//内存块结束位置
			char* endMemoryBlock = nullptr;
			// 这个内存池中对于的各个元素的指针
			PointerNode* pointerNodePointer = nullptr;
			// 链表的下一个指针
			ElementNode* next = nullptr;
		};

		// 内存池中各个元素的指针
		struct PointerNode
		{
			// 元素
			T valuePointer;
			// 链表的下一个指针
			PointerNode* next = nullptr;
		};
	private:
		// 一个内存块能存的元素数量
		size_t p_dataTypeCount;

		//内存块链表起始位置
		ElementNode* p_startElementNode = nullptr;

		// 总的元素结构体指针起始位置
		PointerNode* p_startPointerNode = nullptr;
		// 当前未分配的元素结构体指针
		PointerNode* p_indexPointerNode = nullptr;
		// 总的元素结构体指针终止位置
		PointerNode* p_endPointerNode = nullptr;
		// 元素结构体指针回收链表
		PointerNode* p_freePointerNode = nullptr;

		//检查模式
		CheckMode p_checkMode;
	};

}
