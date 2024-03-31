#pragma once
#include <iostream>
#include <memory>


// Циклический буфер
// Плюсы: буфер фиксированного размера, не нужно постоянно выделять память, проход по буферу быстрый за счет простого перемещения на следующую ячейку
// Минусы: Фиксированный размер

template<typename T, typename Alloc = std::allocator<T>>
class CircularBuffer
{
private:

	Alloc _Alloc;
	T* Ptr;
	std::atomic<bool> IsFull = false;
	std::atomic<bool> IsEmpty = true;;
	std::atomic<size_t> WriteIndex = 0;
	std::atomic<size_t> ReadIndex = 0;
	size_t Size;

public:
	
	template<typename = std::enable_if_t<std::is_pointer_v<T>>>
	void Destroy(const T&)
	{
		while (ReadIndex != WriteIndex)
		{
			if (ReadIndex == Size)
				ReadIndex = 0;

			std::allocator_traits<Alloc>::destroy(_Alloc, Ptr[ReadIndex++]);
		}
	}
	template<typename U, typename = std::enable_if_t<std::is_class_v<U>>>
	void Destroy(const U&)
	{
		while (ReadIndex != WriteIndex)
		{
			if (ReadIndex == Size)
				ReadIndex = 0;

			std::allocator_traits<Alloc>::destroy(_Alloc, &Ptr[ReadIndex++]);
		}
	}
	void Destroy(...){}

	virtual ~CircularBuffer()
	{
		if (Ptr != nullptr)
		{
			Destroy(Ptr[ReadIndex]);

			std::allocator_traits<Alloc>::deallocate(_Alloc, Ptr, Size);
		}
	}
	
	CircularBuffer(size_t _Size) : Size(_Size)
	{
		Ptr = std::allocator_traits<Alloc>::allocate(_Alloc, Size);

		for (int i = 0; i < Size; ++i)
			std::allocator_traits<Alloc>::construct(_Alloc, &Ptr[i]);

	}

	CircularBuffer(size_t _Size, const Alloc& _Alloc) : Size(_Size), _Alloc(_Alloc)
	{
		Ptr = std::allocator_traits<Alloc>::allocate(this->_Alloc, Size);

		for (int i = 0; i < Size; ++i)
			std::allocator_traits<Alloc>::construct(_Alloc, &Ptr[i]);

	}

	CircularBuffer(const CircularBuffer<T, Alloc>& _circularBuffer) : Size(_circularBuffer.Size), WriteIndex(_circularBuffer.WriteIndex.load()), ReadIndex(_circularBuffer.ReadIndex.load()), IsFull(_circularBuffer.IsFull.load()), IsEmpty(_circularBuffer.IsEmpty.load()), _Alloc(std::allocator_traits<Alloc>::select_on_container_copy_construction(_circularBuffer._Alloc))
	{
		//this->_Alloc = std::allocator_traits<Alloc>::select_on_container_copy_construction(_circularBuffer._Alloc);
		
		Ptr = std::allocator_traits<Alloc>::allocate(_Alloc, Size);

		for (int i = 0; i < Size; ++i)
			std::allocator_traits<Alloc>::construct(_Alloc, &Ptr[i], _circularBuffer.Ptr[i]);
	}

	CircularBuffer& operator=(const CircularBuffer<T, Alloc>& _circularBuffer)
	{
		if (this == &_circularBuffer)
			return *this;

		if (std::allocator_traits<Alloc>::propagate_on_container_copy_assignment::value)
		{
			Destroy(Ptr[ReadIndex]);
			std::allocator_traits<Alloc>::deallocate(_Alloc, Ptr, Size);

			this->_Alloc = std::allocator_traits<Alloc>::select_on_container_copy_construction(_circularBuffer._Alloc);
		}
		else
		{
			Destroy(Ptr[ReadIndex]);
			std::allocator_traits<Alloc>::deallocate(_Alloc, Ptr, Size);
		}

		Ptr = std::allocator_traits<Alloc>::allocate(_Alloc, _circularBuffer.Size);

		WriteIndex = _circularBuffer.WriteIndex.load();
		ReadIndex = _circularBuffer.ReadIndex.load();

		IsFull = _circularBuffer.IsFull.load();
		IsEmpty = _circularBuffer.IsEmpty.load();
		Size = _circularBuffer.Size;

		for (int i = 0; i < Size; ++i)
			std::allocator_traits<Alloc>::construct(_Alloc, &Ptr[i], _circularBuffer.Ptr[i]);

		return *this;
	}

	CircularBuffer(CircularBuffer<T, Alloc>&& _circularBuffer) :  Size(std::move(_circularBuffer.Size)), WriteIndex(std::move(_circularBuffer.WriteIndex.load())), ReadIndex(std::move(_circularBuffer.ReadIndex.load())), IsFull(std::move(_circularBuffer.IsFull.load())), IsEmpty(std::move(_circularBuffer.IsEmpty.load()))
	{
		Ptr = std::allocator_traits<Alloc>::allocate(_Alloc, Size);

		for (int i = 0; i < Size; ++i)
			std::allocator_traits<Alloc>::construct(_Alloc, &Ptr[i], std::move(_circularBuffer.Ptr[i]));

		_circularBuffer.Ptr = nullptr;
		_circularBuffer.Size = _circularBuffer.WriteIndex = _circularBuffer.ReadIndex = 0;
	}

	CircularBuffer& operator=(CircularBuffer<T, Alloc>&& _circularBuffer)
	{
		if (this == &_circularBuffer)
			return *this;

		if (std::allocator_traits<Alloc>::propagate_on_container_move_assignment::value)
		{
			Destroy(Ptr[ReadIndex]);
			std::allocator_traits<Alloc>::deallocate(_Alloc, Ptr, Size);

			this->_Alloc = std::move(_circularBuffer._Alloc);
		}
		else
		{
			Destroy(Ptr[ReadIndex]);
			std::allocator_traits<Alloc>::deallocate(_Alloc, Ptr, Size);
		}

		WriteIndex = std::move(_circularBuffer.WriteIndex.load());
		ReadIndex = _circularBuffer.ReadIndex.load();

		IsFull = std::move(_circularBuffer.IsFull.load());
		IsEmpty = std::move(_circularBuffer.IsEmpty.load());
		Size = std::move(_circularBuffer.Size);

		Ptr = std::allocator_traits<Alloc>::allocate(_Alloc, Size);

		for (int i = 0; i < Size; ++i)
			std::allocator_traits<Alloc>::construct(_Alloc, &Ptr[i], std::move(_circularBuffer.Ptr[i]));

		_circularBuffer.Size = _circularBuffer.WriteIndex = _circularBuffer.ReadIndex = 0;
		_circularBuffer.Ptr = nullptr;

		return *this;
	}

	size_t GetSize()
	{
		return Size;
	}

	template<typename T>
	void Write(T&& data)
	{
		if (Ptr == nullptr)
			return;

		if (WriteIndex == ReadIndex && IsEmpty && !IsFull)
		{
			Ptr[WriteIndex++] = std::forward<T>(data);
			IsEmpty = false;
			return;
		}
		else if (WriteIndex == ReadIndex && IsFull && !IsEmpty)
			return;

		Ptr[WriteIndex++] = std::forward<T>(data);

		if (WriteIndex == Size)
			WriteIndex = 0;

		if (WriteIndex == ReadIndex)
			IsFull = true;
		else
			IsFull = false;
	}

	bool Read(T& t)
	{
		if (Ptr == nullptr)
			return false;

		if (WriteIndex == ReadIndex && IsFull && !IsEmpty)
		{
			t = Ptr[ReadIndex++];
			IsFull = false;
			return true;
		}
		else if (WriteIndex == ReadIndex && IsEmpty && !IsFull)
			return false;

		if (ReadIndex == Size)
			ReadIndex = 0;

		if (WriteIndex == ReadIndex)
		{
			IsEmpty = true;
			return false;
		}
		else
		{
			t = Ptr[ReadIndex++];
			IsEmpty = false;
			return true;
		}
	}
};

