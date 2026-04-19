#pragma once

#include <new>
#include <utility>

#include "Wandelt/Defines.hpp"
#include "Wandelt/Memory.hpp"

namespace Wandelt
{

	struct VectorHeader
	{
		Allocator* allocator;
		u64 length;
		u64 capacity;
	};

	// Wrapper around C dynamic array [VectorHeader][T[0] T[1] ...].
	template <typename T>
	class Vector
	{
	public:
		static Vector Create(Allocator* allocator, u64 capacity);

	public:
		void Destroy();

	public:
		T* Data() { return m_Data; }
		const T* Data() const { return m_Data; }
		u64 Length() const { return m_Data ? Header()->length : 0; }
		u64 Capacity() const { return m_Data ? Header()->capacity : 0; }
		bool IsEmpty() const { return Length() == 0; }

	public:
		T& operator[](u64 index)
		{
			ASSERT(m_Data != nullptr, "Vector has not been created!");
			ASSERT(index < Header()->length, "Vector index %llu out of bounds (length %llu)!", index, Header()->length);
			return m_Data[index];
		}

		const T& operator[](u64 index) const
		{
			ASSERT(m_Data != nullptr, "Vector has not been created!");
			ASSERT(index < Header()->length, "Vector index %llu out of bounds (length %llu)!", index, Header()->length);
			return m_Data[index];
		}

		T& Front()
		{
			ASSERT(!IsEmpty(), "Front() called on empty Vector!");
			return m_Data[0];
		}

		const T& Front() const
		{
			ASSERT(!IsEmpty(), "Front() called on empty Vector!");
			return m_Data[0];
		}

		T& Back()
		{
			ASSERT(!IsEmpty(), "Back() called on empty Vector!");
			return m_Data[Header()->length - 1];
		}

		const T& Back() const
		{
			ASSERT(!IsEmpty(), "Back() called on empty Vector!");
			return m_Data[Header()->length - 1];
		}

		void Reserve(u64 newCapacity);
		void Clear();

		void Push(const T& value);
		void Push(T&& value);

		template <typename... Args>
		T& Emplace(Args&&... args);

		void Pop();

	public:
		T* begin() { return m_Data; }
		T* end() { return m_Data + Length(); }
		const T* begin() const { return m_Data; }
		const T* end() const { return m_Data + Length(); }

	private:
		static constexpr u64 s_HeaderOffset = (sizeof(VectorHeader) + alignof(T) - 1) & ~(alignof(T) - 1);

		VectorHeader* Header() { return reinterpret_cast<VectorHeader*>(reinterpret_cast<u8*>(m_Data) - s_HeaderOffset); }

		const VectorHeader* Header() const { return reinterpret_cast<const VectorHeader*>(reinterpret_cast<const u8*>(m_Data) - s_HeaderOffset); }

		void Grow(u64 minCapacity);

	public:
		T* m_Data;
	};

	template <typename T>
	Vector<T> Vector<T>::Create(Allocator* allocator, u64 capacity)
	{
		ASSERT(allocator != nullptr, "Allocator must not be nullptr!");
		ASSERT(capacity > 0, "Initial capacity must be greater than 0!");

		u64 totalSize = s_HeaderOffset + capacity * sizeof(T);
		void* memory  = allocator->Alloc(totalSize);
		ASSERT(memory != nullptr, "Failed to allocate memory for Vector!");

		VectorHeader* header = reinterpret_cast<VectorHeader*>(memory);
		header->allocator    = allocator;
		header->length       = 0;
		header->capacity     = capacity;

		Vector v;
		v.m_Data = reinterpret_cast<T*>(reinterpret_cast<u8*>(memory) + s_HeaderOffset);
		return v;
	}

	template <typename T>
	void Vector<T>::Destroy()
	{
		if (!m_Data)
			return;

		VectorHeader* header = Header();

		for (u64 i = 0; i < header->length; i++) m_Data[i].~T();

		u64 totalSize = s_HeaderOffset + header->capacity * sizeof(T);
		header->allocator->Free(header, totalSize);

		m_Data = nullptr;
	}

	template <typename T>
	void Vector<T>::Reserve(u64 newCapacity)
	{
		ASSERT(m_Data != nullptr, "Vector has not been created!");

		if (newCapacity <= Header()->capacity)
			return;

		Grow(newCapacity);
	}

	template <typename T>
	void Vector<T>::Clear()
	{
		if (!m_Data)
			return;

		VectorHeader* header = Header();
		for (u64 i = 0; i < header->length; i++) m_Data[i].~T();

		header->length = 0;
	}

	template <typename T>
	void Vector<T>::Push(const T& value)
	{
		ASSERT(m_Data != nullptr, "Vector has not been created!");

		VectorHeader* header = Header();
		if (header->length == header->capacity)
		{
			Grow(header->length + 1);
			header = Header();
		}

		new (m_Data + header->length) T(value);
		header->length++;
	}

	template <typename T>
	void Vector<T>::Push(T&& value)
	{
		ASSERT(m_Data != nullptr, "Vector has not been created!");

		VectorHeader* header = Header();
		if (header->length == header->capacity)
		{
			Grow(header->length + 1);
			header = Header();
		}

		new (m_Data + header->length) T(std::move(value));
		header->length++;
	}

	template <typename T>
	template <typename... Args>
	T& Vector<T>::Emplace(Args&&... args)
	{
		ASSERT(m_Data != nullptr, "Vector has not been created!");

		VectorHeader* header = Header();
		if (header->length == header->capacity)
		{
			Grow(header->length + 1);
			header = Header();
		}

		T* slot = new (m_Data + header->length) T(std::forward<Args>(args)...);
		header->length++;

		return *slot;
	}

	template <typename T>
	void Vector<T>::Pop()
	{
		ASSERT(!IsEmpty(), "Pop() called on empty Vector!");

		VectorHeader* header = Header();
		header->length--;
		m_Data[header->length].~T();
	}

	template <typename T>
	void Vector<T>::Grow(u64 minCapacity)
	{
		VectorHeader* header = Header();

		u64 oldCapacity = header->capacity;
		u64 newCapacity = oldCapacity == 0 ? 8 : oldCapacity * 2;
		if (newCapacity < minCapacity)
			newCapacity = minCapacity;

		u64 oldTotalSize = s_HeaderOffset + oldCapacity * sizeof(T);
		u64 newTotalSize = s_HeaderOffset + newCapacity * sizeof(T);

		void* newMemory = header->allocator->Alloc(newTotalSize);
		ASSERT(newMemory != nullptr, "Failed to allocate memory for Vector!");

		VectorHeader* newHeader = reinterpret_cast<VectorHeader*>(newMemory);
		newHeader->allocator    = header->allocator;
		newHeader->length       = header->length;
		newHeader->capacity     = newCapacity;

		T* newData = reinterpret_cast<T*>(reinterpret_cast<u8*>(newMemory) + s_HeaderOffset);
		for (u64 i = 0; i < header->length; i++)
		{
			new (newData + i) T(std::move(m_Data[i]));
			m_Data[i].~T();
		}

		header->allocator->Free(header, oldTotalSize);

		m_Data = newData;
	}

} // namespace Wandelt
