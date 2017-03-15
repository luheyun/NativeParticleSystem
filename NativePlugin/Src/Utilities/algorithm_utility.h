#ifndef ALGORITHM_UTILITY_H
#define ALGORITHM_UTILITY_H

#include <algorithm>
#include <functional>

template<class T, class Func>
inline void repeat (T& type, int count, Func func)
{
	int i;
	for (i=0;i<count;i++)
		func (type);
}

template<class C, class Func>
inline Func for_each (C& c, Func f)
{
	return std::for_each (c.begin (), c.end (), f);
}

template<class C, class Func>
inline void erase_if (C& c, Func f)
{
	c.erase (std::remove_if (c.begin (), c.end (), f), c.end ());
}

template<class C, class T>
inline void erase (C& c, const T& t)
{
	c.erase (std::remove (c.begin (), c.end (), t), c.end ());
}

template<class C, class T>
inline typename C::iterator find (C& c, const T& value)
{
	return std::find (c.begin (), c.end (), value);
}

template<class C, class Pred>
inline typename C::iterator find_if (C& c, Pred p)
{
	return std::find_if (c.begin (), c.end (), p);
}

template <class T, class U>
struct EqualTo
	: std::binary_function<T, U, bool>
{
	bool operator()(const T& x, const U& y) const { return static_cast<bool>(x == y); }
};

// Returns the iterator to the last element
template<class Container>
inline typename Container::iterator last_iterator (Container& container)
{
	Assert (container.begin () != container.end ());
	typename Container::iterator i = container.end ();
	return --i;
}

template<class Container>
inline typename Container::const_iterator last_iterator (const Container& container)
{
	Assert (container.begin () != container.end ());
	typename Container::const_iterator i = container.end ();
	return --i;
}

// Efficient "update or insert" and "find or insert" for STL maps.
// For more details see item 24 on Effective STL.
// They avoid constructing default value only to assign it later
// Can also avoid the temporary allocation that occurs in some STL implementations of .insert (when key is *already* present)
// which can be of even greater significance to performance
template<typename MAP, typename K, typename V>
inline std::pair<typename MAP::iterator,bool> update_or_insert( MAP& m, const K& key, const V& val )
{
	typename MAP::iterator lb = m.lower_bound( key );
	if( lb != m.end() && !m.key_comp()( key, lb->first ) )
	{
		// Key is already present so update value and return (iterator,not-added) pair
		lb->second = val;
		return std::pair<typename MAP::iterator,bool> (lb,false);
	}
	else
	{
		// Key is new. Insert (with hint) and return (iterator, added) pair
		typename MAP::iterator add = m.insert( lb, std::pair<K,V>(key,val) );
		return std::pair<typename MAP::iterator,bool> (add,true);
	}
}

// find_or_insert
// Same as update_or_insert but does NOT update value
template<typename MAP, typename K, typename V>
inline std::pair<typename MAP::iterator,bool> find_or_insert( MAP& m, const K& key, const V& val)
{
	typename MAP::iterator lb = m.lower_bound( key );
	if( lb != m.end() && !m.key_comp()( key, lb->first ))
	{
		// Key is already present so return (iterator,not-added) pair
		return std::pair<typename MAP::iterator,bool>(lb,false);
	}
	else
	{
		// Key is new. Insert (with hint) and return (iterator, added) pair
		typename MAP::iterator add = m.insert( lb, std::pair<K,V>(key,val) );
		return std::pair<typename MAP::iterator,bool>(add,true);
	}
}

template<class ForwardIterator>
inline bool is_sorted (ForwardIterator begin, ForwardIterator end)
{
	for (ForwardIterator next = begin; begin != end && ++next != end; ++begin)
		if (*next < *begin)
			return false;
	
	return true;
}

template<class ForwardIterator, class Predicate>
inline bool is_sorted (ForwardIterator begin, ForwardIterator end, Predicate pred)
{
	for (ForwardIterator next = begin; begin != end && ++next != end; ++begin)
		if (pred(*next, *begin))
			return false;
	
	return true;
}

template <class PairType>
struct select1st : public std::unary_function<PairType, typename PairType::first_type>
{
	typedef std::unary_function<PairType, typename PairType::first_type> BaseClassType;

	typedef typename BaseClassType::result_type     result_type;
	typedef typename BaseClassType::argument_type   argument_type;
	typedef PairType                                value_type;

	const result_type& operator()(const argument_type& thePair) const
	{
		return thePair.first;
	}
};

template <class PairType>
struct select2nd : public std::unary_function<PairType, typename PairType::second_type>
{
	typedef std::unary_function<PairType, typename PairType::second_type> BaseClassType;

	typedef typename BaseClassType::result_type     result_type;
	typedef typename BaseClassType::argument_type   argument_type;
	typedef PairType                                value_type;

	const result_type& operator()(const argument_type& thePair) const
	{
		return thePair.second;
	}
};

template <typename T, typename Iterator, typename Predicate>
inline T* BinarySearch(Iterator first, Iterator last, const T& value, const Predicate& predicate = Predicate())
{
	Iterator foundValue = std::lower_bound(first, last, value);

	if (foundValue == last)
		return NULL;

	if (predicate(*foundValue, value))
	{
		return &*foundValue;
	}
	else
	{
		return NULL;
	}
}

template <typename T, typename Iterator>
inline T* BinarySearch(Iterator first, Iterator last, const T& value)
{
	return BinarySearch(first, last, value, EqualTo<T, T>());
}

template <typename T, typename Container, typename Predicate>
inline T* BinarySearch(const Container& container, const T& value, const Predicate& predicate = Predicate())
{
	return BinarySearch(container.begin(), container.end(), value, predicate);
}

template <typename T, typename Container>
inline T* BinarySearch(const Container& container, const T& value)
{
	return BinarySearch(container.begin(), container.end(), value, EqualTo<T, T>());
}

#endif
