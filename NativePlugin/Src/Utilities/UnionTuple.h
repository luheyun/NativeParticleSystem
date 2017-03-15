#ifndef UNION_TUPLE_H_
#define UNION_TUPLE_H_

template<typename A, typename B>
union UnionTuple
{
	typedef A frist_type;
	typedef B second_type;

	A first;
	B second;
};

template<typename B, typename A>
inline UnionTuple<A, B>& AliasAs(A& a)
{
	return reinterpret_cast<UnionTuple<A, B>&>(a);
}

template<typename B, typename A>
inline UnionTuple<A, B> const& AliasAs(A const& a)
{
	return reinterpret_cast<UnionTuple<A, B> const&>(a);
}

#endif