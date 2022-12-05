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
			return reinterpret_cast<T*>(beforeElementNode->next->sB->get(requestSize));
		}

		void deallocate(void* pointer)
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
				p_blockPointer = reinterpret_cast<void*>(new char[p_blockPointerSize] {0});
				p_startPointerNode = new PointerNode[p_blockPointerSize];
				p_indexPointerNode = p_startPointerNode;
				p_lastPointerNode = p_startPointerNode + p_blockPointerSize;
				p_indexPointerNode->valuePointer = p_blockPointer;
				p_indexPointerNode->localBlockSize = p_blockPointerSize;
			}
			~SingleBlock()
			{
				delete[] reinterpret_cast<char*>(p_blockPointer);
				delete[] p_startPointerNode;
			}

			void* get(size_t size)
			{
				if (p_freePointerNode != nullptr)
				{
					PointerNode* indexPointerNode = p_freePointerNode;
					//PointerNode* beforePointerNode = nullptr;
					while (indexPointerNode != nullptr)
					{
						if (indexPointerNode->localBlockSize == size)
						{
							p_freePointerNode = indexPointerNode->next;
							indexPointerNode->next = nullptr;
							return indexPointerNode->valuePointer;
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
							//indexPointerNode->next = nullptr;
							newPointerNode->localBlockSize = indexPointerNode->localBlockSize - size;
							newPointerNode->valuePointer = reinterpret_cast<char*>(indexPointerNode->valuePointer) + size;
							indexPointerNode->localBlockSize = size;
							newPointerNode->next = p_freePointerNode->next;
							p_freePointerNode = newPointerNode;
							indexPointerNode->next = nullptr;
							return indexPointerNode->valuePointer;
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
					//indexPointerNode->next = nextPointerNode;
					nextPointerNode->localBlockSize = indexPointerNode->localBlockSize - size;
					indexPointerNode->localBlockSize = size;
					nextPointerNode->valuePointer = reinterpret_cast<char*>(indexPointerNode->valuePointer) + size;
					return indexPointerNode->valuePointer;
				}
				else
				{
					return nullptr;
				}
			}

			int postBack(void* back)
			{
				PointerNode* indexPointerNode = p_startPointerNode;
				while (indexPointerNode != nullptr && indexPointerNode < p_indexPointerNode)
				{
					if (indexPointerNode->valuePointer == back)
					{
						break;
					}
					indexPointerNode++;
				}
				if (indexPointerNode == p_indexPointerNode || indexPointerNode == nullptr)
					return -1;

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
				size_t localBlockSize = 0;
				void* valuePointer = nullptr;
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
}
