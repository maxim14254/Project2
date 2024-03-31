#pragma once
#include <iostream>
#include <memory>
#include <mutex>


// Циклический буфер, реализованный посредством двухсвязного списка
// Плюсы: Возможность изменения размера буфера в процессе выполнения
// Минусы: Требует больше памяти за счет 2 указателей, работает медленнее за счет постоянного выделения(или удаления) памяти, а также за счет постоянного разыменовывания указателя

template<typename T, typename Alloc = std::allocator<T>>
class RingList
{
private:
	struct Node
	{
		Node* Next = nullptr;
		Node* Prev = nullptr;
		T Data;

		Node(const T& data) : Data(data){ }
		Node(T&& data) : Data(std::move(data)) { }

		~Node()
		{
			Destroy(Data);
		}

		template<typename = std::enable_if_t<std::is_pointer_v<T>>>
		void Destroy(const T&)
		{
			delete Data;
		}
		void Destroy(...) {}
	};

	Node* Current = nullptr;
	std::recursive_mutex Current_mutex;
	using AllocNode = typename Alloc::template rebind<Node>::other;
	AllocNode _Alloc;
	size_t Size = 0;

public:

	RingList() = default;

	RingList(const Alloc& _Alloc) : _Alloc(_Alloc)
	{

	}

	RingList(const RingList<T, Alloc>& _ringList) : _Alloc(std::allocator_traits<AllocNode>::select_on_container_copy_construction(_ringList._Alloc))
	{
		for (int i = 0; i < _ringList.Size; ++i)
		{
			PushNext(_ringList.Current->Data);
		}
	}

	RingList& operator=(const RingList<T, Alloc>& _ringList)
	{
		if (this == &_ringList)
			return *this;

		if (std::allocator_traits<AllocNode>::propagate_on_container_copy_assignment::value)
		{
			Node* temp = Current;
			for (int i = 0; i < Size; ++i)
			{
				std::allocator_traits<AllocNode>::destroy(_Alloc, temp);
				temp = temp->Next;
				std::allocator_traits<AllocNode>::deallocate(_Alloc, Current, 1);
				Current = temp;
			}

			this->_Alloc = std::allocator_traits<AllocNode>::select_on_container_copy_construction(_ringList._Alloc);
		}
		else
		{
			Node* temp = Current;
			for (int i = 0; i < Size; ++i)
			{
				std::allocator_traits<AllocNode>::destroy(_Alloc, temp);
				temp = temp->Next;
				std::allocator_traits<AllocNode>::deallocate(_Alloc, Current, 1);
				Current = temp;
			}
		}

		Size = 0;

		Current = nullptr;

		for (int i = 0; i < _ringList.Size; ++i)
		{
			PushNext(_ringList.Current->Data);
		}

		return *this;
	}

	RingList(RingList<T, Alloc>&& _ringList)
	{
		for (int i = 0; i < _ringList.Size; ++i)
		{
			PushNext(std::move(_ringList.Current->Data));
			_ringList.Current = _ringList.Current->Next;
		}

		_ringList.Current = nullptr;
		_ringList.Size = 0;
	}

	RingList& operator=(RingList<T, Alloc>&& _ringList)
	{
		if (this == &_ringList)
			return *this;

		if (std::allocator_traits<AllocNode>::propagate_on_container_move_assignment::value)
		{
			Node* temp = Current;
			for (int i = 0; i < Size; ++i)
			{
				std::allocator_traits<AllocNode>::destroy(_Alloc, temp);
				temp = temp->Next;
				std::allocator_traits<AllocNode>::deallocate(_Alloc, Current, 1);
				Current = temp;
			}

			this->_Alloc = std::move(_ringList._Alloc);
		}
		else
		{
			Node* temp = Current;
			for (int i = 0; i < Size; ++i)
			{
				std::allocator_traits<AllocNode>::destroy(_Alloc, temp);
				temp = temp->Next;
				std::allocator_traits<AllocNode>::deallocate(_Alloc, Current, 1);
				Current = temp;
			}
		}

		Size = 0;

		Current = nullptr;

		for (int i = 0; i < _ringList.Size; ++i)
		{
			PushNext(std::move(_ringList.Current->Data));
		}

		return *this;
	}

	virtual ~RingList()
	{
		Node* temp = Current;
		for (int i = 0; i < Size; ++i)
		{
			std::allocator_traits<AllocNode>::destroy(_Alloc, temp);
			temp = temp->Next;
			std::allocator_traits<AllocNode>::deallocate(_Alloc, Current, 1);
			Current = temp;
		}
	}
	
	size_t GetSize()
	{
		return Size;
	}

	template<typename T>
	void PushNext(T&& data)
	{
		std::lock_guard<std::recursive_mutex> g(Current_mutex);

		if (Current == nullptr)
		{
			Current = std::allocator_traits<AllocNode>::allocate(_Alloc, 1);
			std::allocator_traits<AllocNode>::construct(_Alloc, Current, std::forward<T>(data));

			Current->Next = Current;
			Current->Prev = Current;
			++Size;
			return;
		}

		Node* node = std::allocator_traits<AllocNode>::allocate(_Alloc, 1);
		std::allocator_traits<AllocNode>::construct(_Alloc, node, std::forward<T>(data));

		node->Next = Current->Next;
		node->Prev = Current;
		Current->Next = node;
		node->Next->Prev = node;
		Current = node;
		++Size;
	}

	bool ReadCurrent(T& t)
	{
		std::lock_guard<std::recursive_mutex> g(Current_mutex);

		if (Current != nullptr)
		{
			t = Current->Data;
			Current = Current->Next;
			return true;
		}

		return false;
	}

	bool DeleteCurrent(size_t count)
	{
		std::lock_guard<std::recursive_mutex> g(Current_mutex);

		if (Current != nullptr)
		{
			Node* temp = Current;

			for (int i = 0; i < count && Size > 0; ++i)
			{
				std::allocator_traits<AllocNode>::destroy(_Alloc, temp);
				Current = temp->Next;
				temp->Prev->Next = Current;
				Current->Prev = temp->Prev;
				std::allocator_traits<AllocNode>::deallocate(_Alloc, temp, 1);

				--Size;

				if (Size == 0)
					Current = nullptr;

				temp = Current;
			}

			return true;
		}

		return false;
	}

	void PushRound(T* vec, size_t count)
	{
		std::lock_guard<std::recursive_mutex> g(Current_mutex);

		for (int i = 0; i < count; ++i)
		{
			PushNext(std::forward<T>(vec[i]));
		}
	}
};
