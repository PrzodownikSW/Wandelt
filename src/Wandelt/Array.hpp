#pragma once

#include <new>
#include <utility>

#include "Wandelt/Defines.hpp"
#include "Wandelt/Memory.hpp"

namespace Wandelt
{

	template <typename T>
	class Array
	{
	public:
		static Array WithCapacity(Allocator* alloc, u64 capacity);

		Array() = default;
		explicit Array(Allocator* alloc) : m_Allocator(alloc) {}

		~Array();

		NONCOPYABLE(Array);

		Array(Array&& other) noexcept;
		Array& operator=(Array&& other) noexcept;

	public:
		T* Data() { return m_Data; }
		const T* Data() const { return m_Data; }
		u64 Length() const { return m_Len; }
		u64 Capacity() const { return m_Capacity; }
		bool IsEmpty() const { return m_Len == 0; }

	public:
		T& operator[](u64 index)
		{
			ASSERT(index < m_Len, "Array index %llu out of bounds (length %llu)!", index, m_Len);
			return m_Data[index];
		}

		const T& operator[](u64 index) const
		{
			ASSERT(index < m_Len, "Array index %llu out of bounds (length %llu)!", index, m_Len);
			return m_Data[index];
		}

		T& Front()
		{
			ASSERT(m_Len > 0, "Front() called on empty Array!");
			return m_Data[0];
		}

		const T& Front() const
		{
			ASSERT(m_Len > 0, "Front() called on empty Array!");
			return m_Data[0];
		}

		T& Back()
		{
			ASSERT(m_Len > 0, "Back() called on empty Array!");
			return m_Data[m_Len - 1];
		}

		const T& Back() const
		{
			ASSERT(m_Len > 0, "Back() called on empty Array!");
			return m_Data[m_Len - 1];
		}

		void Reserve(u64 newCapacity);
		void Resize(u64 newLen);
		void Clear();

		void Push(const T& value);
		void Push(T&& value);

		template <typename... Args>
		T& Emplace(Args&&... args);

		void Pop();

	public:
		T* begin() { return m_Data; }
		T* end() { return m_Data + m_Len; }
		const T* begin() const { return m_Data; }
		const T* end() const { return m_Data + m_Len; }

	private:
		void Grow(u64 minCapacity);

	private:
		Allocator* m_Allocator = nullptr;
		T* m_Data              = nullptr;
		u64 m_Len              = 0;
		u64 m_Capacity         = 0;
	};

	template <typename T>
	Array<T> Array<T>::WithCapacity(Allocator* alloc, u64 capacity)
	{
		Array a(alloc);
		a.Reserve(capacity);

		return a;
	}

	template <typename T>
	Array<T>::~Array()
	{
		if (!m_Allocator)
			return;

		for (u64 i = 0; i < m_Len; i++) m_Data[i].~T();

		if (m_Data)
			m_Allocator->Free(m_Data, m_Capacity * sizeof(T));
	}

	template <typename T>
	Array<T>::Array(Array&& other) noexcept : m_Allocator(other.m_Allocator), m_Data(other.m_Data), m_Len(other.m_Len), m_Capacity(other.m_Capacity)
	{
		other.m_Allocator = nullptr;
		other.m_Data      = nullptr;
		other.m_Len       = 0;
		other.m_Capacity  = 0;
	}

	template <typename T>
	Array<T>& Array<T>::operator=(Array&& other) noexcept
	{
		if (this != &other)
		{
			for (u64 i = 0; i < m_Len; i++) m_Data[i].~T();

			if (m_Allocator && m_Data)
				m_Allocator->Free(m_Data, m_Capacity * sizeof(T));

			m_Allocator = other.m_Allocator;
			m_Data      = other.m_Data;
			m_Len       = other.m_Len;
			m_Capacity  = other.m_Capacity;

			other.m_Allocator = nullptr;
			other.m_Data      = nullptr;
			other.m_Len       = 0;
			other.m_Capacity  = 0;
		}

		return *this;
	}

	template <typename T>
	void Array<T>::Reserve(u64 newCapacity)
	{
		if (newCapacity <= m_Capacity)
			return;

		Grow(newCapacity);
	}

	template <typename T>
	void Array<T>::Resize(u64 newLen)
	{
		if (newLen > m_Capacity)
			Grow(newLen);

		if (newLen > m_Len)
		{
			for (u64 i = m_Len; i < newLen; i++) new (m_Data + i) T();
		}
		else
		{
			for (u64 i = newLen; i < m_Len; i++) m_Data[i].~T();
		}

		m_Len = newLen;
	}

	template <typename T>
	void Array<T>::Clear()
	{
		for (u64 i = 0; i < m_Len; i++) m_Data[i].~T();

		m_Len = 0;
	}

	template <typename T>
	void Array<T>::Push(const T& value)
	{
		if (m_Len == m_Capacity)
			Grow(m_Len + 1);

		new (m_Data + m_Len) T(value);
		m_Len++;
	}

	template <typename T>
	void Array<T>::Push(T&& value)
	{
		if (m_Len == m_Capacity)
			Grow(m_Len + 1);

		new (m_Data + m_Len) T(std::move(value));
		m_Len++;
	}

	template <typename T>
	template <typename... Args>
	T& Array<T>::Emplace(Args&&... args)
	{
		if (m_Len == m_Capacity)
			Grow(m_Len + 1);

		T* slot = new (m_Data + m_Len) T(std::forward<Args>(args)...);
		m_Len++;

		return *slot;
	}

	template <typename T>
	void Array<T>::Pop()
	{
		ASSERT(m_Len > 0, "Pop() called on empty Array!");
		m_Len--;
		m_Data[m_Len].~T();
	}

	template <typename T>
	void Array<T>::Grow(u64 minCapacity)
	{
		ASSERT(m_Allocator != nullptr, "Array has no allocator!");

		u64 newCapacity = m_Capacity == 0 ? 8 : m_Capacity * 2;
		if (newCapacity < minCapacity)
			newCapacity = minCapacity;

		T* newData = (T*)m_Allocator->Alloc(newCapacity * sizeof(T));

		for (u64 i = 0; i < m_Len; i++)
		{
			new (newData + i) T(std::move(m_Data[i]));
			m_Data[i].~T();
		}

		if (m_Data)
			m_Allocator->Free(m_Data, m_Capacity * sizeof(T));

		m_Data     = newData;
		m_Capacity = newCapacity;
	}

} // namespace Wandelt
