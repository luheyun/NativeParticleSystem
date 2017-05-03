#pragma once

#include "Allocator/MemoryMacros.h"
#include <memory> // std::uninitialized_fill
#include "Misc/AllocatorLabels.h"

// dynamic_array - simplified version of std::vector<T>
//
// features:
//  . always uses memcpy for copying elements. Your data structures must be simple and can't have internal pointers / rely on copy constructor.
//  . EASTL like push_back(void) implementation
//		Existing std STL implementations implement insertion operations by copying from an element.
//		For example, resize(size() + 1) creates a throw-away temporary object.
//		There is no way in existing std STL implementations to add an element to a container without implicitly or
//			explicitly providing one to copy from (aside from some existing POD optimizations).
//		For expensive-to-construct objects this creates a potentially serious performance problem.
//  . grows X2 on reallocation
//  . small code footprint
//  . clear actually deallocates memory
//  . resize does NOT initialize members!
//
//	Changelog:
//		Added pop_back()
//		Added assign()
//		Added clear() - frees the data, use resize(0) to clear w/o freeing
//		zero allocation for empty array
//
// If we need an array fixed size is known at compilation time, consider using "Runtime/Utilities/fixed_array.h".

template<typename T>
struct AlignOfType
{
	enum { align = ALIGN_OF(T) };
};



template <typename T, size_t ALIGN = AlignOfType<T>::align>
struct dynamic_array
{
public:
	enum { align = ALIGN };

	typedef		T*									iterator;
	typedef		const T*							const_iterator;
	typedef		T									value_type;
	typedef		size_t								size_type;
	typedef		size_t								difference_type;
	typedef		T&									reference;
	typedef		const T&							const_reference;

public:

	dynamic_array() : m_data(NULL), m_size(0), m_capacity(0)
	{
		m_label = MemLabelId(kMemDynamicArrayId);
	}

	dynamic_array(MemLabelId label) : m_data(0), m_label(label), m_size(0), m_capacity(0)
	{
	}

	explicit dynamic_array (size_t size)
	:  m_size(size), m_capacity (size)
	{
		m_data = allocate (size);
	}

	dynamic_array(size_t size, T const& init_value, MemLabelRef label)
		: m_label(label), m_size(size), m_capacity(size)
	{
		m_data = allocate (size);
		std::uninitialized_fill (m_data, m_data + size, init_value);
	}

	~dynamic_array()
	{
		if (owns_data())
			deallocate(m_data);
	}

	dynamic_array(const dynamic_array& other) : m_label(other.m_label), m_size(0), m_capacity(0)
	{
		m_data = NULL;
		assign(other.begin(), other.end());
	}

	dynamic_array& operator=(const dynamic_array& other)
	{
		// should not allocate memory unless we have to
		if (&other != this)
			assign(other.begin(), other.end());
		return *this;
	}

	void clear()
	{
		if (owns_data())
			deallocate(m_data);
		m_data = NULL;
		m_size = 0;
		m_capacity = 0;
	}

	void assign(const_iterator begin, const_iterator end)
	{
		resize_uninitialized(end-begin);
		memcpy(m_data, begin, m_size * sizeof(T));
	}

	iterator erase(iterator input_begin, iterator input_end)
	{
		size_t leftOverSize = end() - input_end;
		memmove(input_begin, input_end, leftOverSize * sizeof(T));
		m_size -= input_end - input_begin;
		return input_begin;
	}

	iterator erase(iterator it)
	{
		return erase(it, it+1);
	}

	iterator erase_swap_back(iterator it)
	{
		m_size--;
		memcpy(it, end(), sizeof(T));
		return it;
	}

	iterator insert(iterator insert_before, const_iterator input_begin, const_iterator input_end)
	{

		// resize (make sure that insertBefore does not get invalid in the meantime because of a reallocation)
		size_t insert_before_index = insert_before - begin();
		size_t elements_to_be_moved = size() - insert_before_index;
		resize_uninitialized((input_end - input_begin) + size(), true);
		insert_before = begin() + insert_before_index;

		size_t insertsize = input_end - input_begin;
		// move to the end of where the inserted data will be
		memmove(insert_before + insertsize, insert_before, elements_to_be_moved * sizeof(T));
		// inject input data in the hole we just created
		memcpy(insert_before, input_begin, insertsize * sizeof(T));

		return insert_before;
	}

	iterator insert(iterator insertBefore, const T& t)   { return insert(insertBefore, &t, &t + 1); }

	// Returns the memory to the object.
	// This does not call the constructor for the newly added element.
	// You are expected to initialize all member variables of the returned data.
	T& push_back()
	{
		if (++m_size > capacity())
			reserve(std::max<size_t>(capacity()*2, 1));
		return back();
	}

	// push_back but it also calls the constructor for the newly added element.
	T& push_back_construct()
	{
		if (++m_size > capacity())
			reserve(std::max<size_t>(capacity()*2, 1));
		// construct
		T* ptr = &back();
		new (ptr) T ;
		return *ptr;
	}

	// push_back but assigns /t/ to the newly added element.
	void push_back(const T& t)
	{
		push_back() = t;
	}

	void pop_back()
	{
		m_size--;
	}

	void resize_uninitialized(size_t size, bool double_on_resize = false)
	{
		m_size = size;
		if (m_size <= capacity())
			return;

		if(double_on_resize && size < capacity()*2)
			size = capacity()*2;
		reserve(size);
	}

	void resize_initialized(size_t size, const T& t = T(), bool double_on_resize = false)
	{
		if (size > capacity())
		{
			size_t requested_size = size;
			if(double_on_resize && size < capacity()*2)
				requested_size = capacity()*2;
			reserve(requested_size);
		}

		if (size > m_size)
			std::uninitialized_fill (m_data + m_size, m_data + size, t);
		m_size = size;
	}

	void reserve(size_t inCapacity)
	{
		if (capacity() >= inCapacity)
			return;

		if (owns_data())
		{
			m_capacity = inCapacity;
			m_data = reallocate(m_data, inCapacity);
		}
		else
		{
			T* newData = allocate(inCapacity);
			memcpy(newData, m_data, m_size * sizeof(T));

			// Invalidate old non-owned data, since using the data from two places is most likely a really really bad idea.
			#if DEBUGMODE
			memset(m_data, 0xCD, capacity() * sizeof(T));
			#endif

			m_capacity = inCapacity; // and clear reference bit
			m_data = newData;
		}
	}

	void assign_external (T* begin, T* end)
	{
		if (owns_data())
			deallocate(m_data);
		m_size = m_capacity = reinterpret_cast<value_type*> (end) - reinterpret_cast<value_type*> (begin);
		Assert(m_size < k_reference_bit);
		m_capacity |= k_reference_bit;
		m_data = begin;
	}

	void set_owns_data (bool ownsData)
	{
		if (ownsData)
			m_capacity &= ~k_reference_bit;
		else
			m_capacity |= k_reference_bit;
	}

	void shrink_to_fit()
	{
		if (owns_data())
		{
			m_capacity = m_size;
			m_data = reallocate(m_data, m_size);
		}
	}

	const T& back() const { return m_data[m_size - 1]; }
	const T& front() const { return m_data[0]; }

	T& back() { return m_data[m_size - 1]; }
	T& front() { return m_data[0]; }

	T* data () { return m_data; }
	T const* data () const { return m_data; }

	bool empty () const { return m_size == 0; }
	size_t size() const { return m_size; }
	size_t capacity() const { return m_capacity & ~k_reference_bit; }

	T const& operator[] (size_t index) const { return m_data[index]; }
	T& operator[] (size_t index) { return m_data[index]; }

	T const* begin() const { return m_data; }
	T* begin() { return m_data; }

	T const* end() const { return m_data + m_size; }
	T* end() { return m_data + m_size; }

	bool owns_data() { return (m_capacity & k_reference_bit) == 0; }

	bool equals(const dynamic_array& other) const
	{
		if(m_size != other.m_size)
			return false;

		for( int i = 0; i < m_size; i++)
		{
			if (!(m_data[i] == other.m_data[i]))
				return false;
		}

		return true;
	}

private:

	static const size_t k_reference_bit = (size_t)1 << (sizeof (size_t) * 8 - 1);

	T* allocate (size_t size)
	{
		// If you are getting this error then you are trying to allocate memory for an incomplete type
		//return static_cast<T*> (malloc(size * sizeof(T)));
		return static_cast<T*> (UNITY_MALLOC_ALIGNED(m_label, size * sizeof(T), align));
	}

	void deallocate (T* data)
	{
		//delete data;
		UNITY_FREE(m_label, data);
	}

	T* reallocate (T* data, size_t size)
	{
		// If you are getting this error then you are trying to allocate memory for an incomplete type
		//return static_cast<T*> (realloc(data, size * sizeof(T)));
		int alignment = static_cast<int>(align);
		return static_cast<T*> (UNITY_REALLOC_ALIGNED(m_label, data, size * sizeof(T), alignment));
	}

	T*          m_data;
	MemLabelId  m_label;
	size_t      m_size;
	size_t      m_capacity;
};
