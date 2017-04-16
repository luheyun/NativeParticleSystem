#pragma once

#include <emscripten/threading.h>

static inline void atomic_thread_fence (memory_order_relaxed_t)
{
 	atomic_thread_fence(memory_order_relaxed);
}

static inline void atomic_thread_fence (memory_order_acquire_t)
{
 	atomic_thread_fence(memory_order_acquire);
}

static inline void atomic_thread_fence (memory_order_release_t)
{
 	atomic_thread_fence(memory_order_release);
}

static inline void atomic_thread_fence (memory_order_acq_rel_t)
{
 	atomic_thread_fence(memory_order_acq_rel);
}

static inline void atomic_thread_fence (int /* memory_order_seq_cst_t */)
{
 	atomic_thread_fence(memory_order_seq_cst);
}

static inline atomic_word atomic_load_explicit(const volatile atomic_word* p, int)
{
	return emscripten_atomic_load_u32((void*)p);
}

static inline void atomic_store_explicit(volatile atomic_word* p, atomic_word v, int)
{
	emscripten_atomic_store_u32((void*)p, v);
}

static inline void atomic_store_explicit(volatile atomic_word* p, atomic_word val, memory_order_seq_cst_t)
{
	emscripten_atomic_store_u32((void*)p, val);
}

static inline atomic_word atomic_exchange_explicit(volatile atomic_word* p, atomic_word val, int)
{
	return emscripten_atomic_exchange_u32((void*)p, val);
}

static inline atomic_word2 atomic_exchange_explicit(volatile atomic_word2* p, atomic_word2 val, int)
{
	atomic_word2 w;
	w.v = emscripten_atomic_exchange_u64((void*)p, val.v);
	return w;
}

static inline bool atomic_compare_exchange_strong_explicit(volatile atomic_word* p, atomic_word* oldval, atomic_word newval, int, int)
{
	atomic_word prev = emscripten_atomic_cas_u32((void*)p, *oldval, newval);
	if (prev == *oldval)
		return true;
	else
	{
		*oldval = prev;
		return false;
	}
}

static inline atomic_word2 atomic_load_explicit(const volatile atomic_word2* p, int)
{
	atomic_word2 w;
	w.v = emscripten_atomic_load_u64((void*)p);
	return w;
}

static inline void atomic_store_explicit(volatile atomic_word2* p, atomic_word2 v, int)
{
	emscripten_atomic_store_u64((void*)p, v.v);
}

static inline void atomic_init_safe_explicit(volatile atomic_word2* p, atomic_word v, int x)
{
	atomic_word2 w;
	w.lo = v;
	w.hi = 0;
	atomic_store_explicit(p, w, x);
}

static inline bool atomic_compare_exchange_strong_explicit(volatile atomic_word2* p, atomic_word2* oldval, atomic_word2 newval, int, int)
{
	uint64_t prev = emscripten_atomic_cas_u64((void*)p, oldval->v, newval.v);
	if (prev == oldval->v)
		return true;
	else
	{
		oldval->v = prev;
		return false;
	}
}

static inline bool atomic_compare_exchange_weak_explicit (volatile atomic_word* p, atomic_word* oldval, atomic_word newval, int, int)
{
	return atomic_compare_exchange_strong_explicit(p, oldval, newval, ::memory_order_seq_cst, ::memory_order_seq_cst);
}

static inline bool atomic_compare_exchange_safe_explicit (volatile atomic_word2* p, atomic_word2* oldval, atomic_word newlo, int, int)
{
	atomic_word2 newval;
	newval.lo = newlo;
	newval.hi = oldval->hi + 1;
	return atomic_compare_exchange_strong_explicit (p, oldval, newval, memory_order_seq_cst, memory_order_seq_cst);
}

static inline atomic_word atomic_exchange_safe_explicit (volatile atomic_word2* p, atomic_word v, int)
{
	atomic_word2 oldval = atomic_load_explicit (p, memory_order_relaxed);
	atomic_word2 newval;
	do
	{
		newval.lo = v; newval.hi = oldval.hi + 1;
	}
	while (!atomic_compare_exchange_strong_explicit (p, &oldval, newval, memory_order_seq_cst, memory_order_seq_cst));
	return oldval.lo;
}

static inline atomic_word atomic_fetch_add_explicit (volatile atomic_word* p, atomic_word v, memory_order_relaxed_t)
{
	return atomic_fetch_add_explicit(p, v, memory_order_relaxed);
}

static inline atomic_word atomic_fetch_add_explicit (volatile atomic_word* p, atomic_word v, memory_order_acquire_t)
{
	return atomic_fetch_add_explicit(p, v, memory_order_acquire);
}

static inline atomic_word atomic_fetch_add_explicit (volatile atomic_word* p, atomic_word v, memory_order_release_t)
{
	return atomic_fetch_add_explicit(p, v, memory_order_release);
}

static inline atomic_word atomic_fetch_add_explicit (volatile atomic_word* p, atomic_word v, memory_order_acq_rel_t)
{
	return atomic_fetch_add_explicit(p, v, memory_order_acq_rel);
}

static inline atomic_word atomic_fetch_add_explicit (volatile atomic_word* p, atomic_word v, int /* memory_order_seq_cst_t */)
{
	return atomic_fetch_add_explicit(p, v, memory_order_seq_cst);
}

static inline atomic_word atomic_fetch_sub_explicit (volatile atomic_word* p, atomic_word v, memory_order_relaxed_t)
{
	return atomic_fetch_sub_explicit(p, v, memory_order_relaxed);
}

static inline atomic_word atomic_fetch_sub_explicit (volatile atomic_word* p, atomic_word v, memory_order_acquire_t)
{
	return atomic_fetch_sub_explicit(p, v, memory_order_acquire);
}

static inline atomic_word atomic_fetch_sub_explicit (volatile atomic_word* p, atomic_word v, memory_order_release_t)
{
	return atomic_fetch_sub_explicit(p, v, memory_order_release);
}

static inline atomic_word atomic_fetch_sub_explicit (volatile atomic_word* p, atomic_word v, memory_order_acq_rel_t)
{
	return atomic_fetch_sub_explicit(p, v, memory_order_acq_rel);
}

static inline atomic_word atomic_fetch_sub_explicit (volatile atomic_word* p, atomic_word v, int /* memory_order_seq_cst_t */)
{
	return atomic_fetch_sub_explicit(p, v, memory_order_seq_cst);
}
