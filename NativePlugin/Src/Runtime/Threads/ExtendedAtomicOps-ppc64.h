#define ASM_LWSYNC      "lwsync\n\t"
#define ASM_SYNC        "sync\n\t"
#define ASM_ISYNC       "isync\n\t"
//#define ASM_MOVCC0(r)   "mfcrf  " #r ", 1\n\t"
//#define ASM_MOVCC0(r)   "mfcr " #r "\n\trlwinm " #r "," #r ", 3, 31, 31\n\t"
#define ASM_MOVCC0(r)   "bne- 1f\n\tli %" #r ", 1\n\t"
#define ASM_MOV1(r)     "li     %" #r ", 1\n\t"
#define ASM_LABEL(i)    #i ":\n\t"

static inline void atomic_pause ()
{
}

static inline void atomic_thread_fence (memory_order_relaxed_t)
{
}

static inline void atomic_thread_fence (memory_order_release_t)
{
	__asm__ __volatile__
	(
		ASM_LWSYNC : : : "memory"
    );
}

static inline void atomic_thread_fence (memory_order_acquire_t)
{
	__asm__ __volatile__
	(
		ASM_LWSYNC : : : "memory"
    );
}

static inline void atomic_thread_fence (memory_order_acq_rel_t)
{
	__asm__ __volatile__
	(
		ASM_LWSYNC : : : "memory"
    );
}

static inline void atomic_thread_fence (int /* memory_order_seq_cst_t */)
{
	__asm__ __volatile__
	(
		ASM_SYNC : : : "memory"
    );
}

/*
 *	int support
 */

static inline int atomic_load_explicit (const volatile int *p, memory_order_relaxed_t)
{
	return *p;
}

static inline int atomic_load_explicit (const volatile int *p, memory_order_acquire_t)
{
	int res;
	__asm__ __volatile__
	(
		"lwz	%0, %1\n\t"
		"twi	0, %0, 0\n\t"
		ASM_ISYNC
		: "=r" (res)
		: "m" (*p)
		: "memory"
	);
	return res;
}

static inline int atomic_load_explicit (const volatile int *p, int /* memory_order_seq_cst_t */)
{
	int res;
	__asm__ __volatile__
	(
        ASM_SYNC
		"lwz	%0, %1\n\t"
		"twi	0, %0, 0\n\t"
		ASM_ISYNC
		: "=r" (res)
		: "m" (*p)
		: "memory"
	);
	return res;
}

static inline void atomic_store_explicit (volatile int *p, int v, memory_order_relaxed_t)
{
	*p = v;
}

static inline void atomic_store_explicit (volatile int *p, int v, memory_order_release_t)
{
	__asm__ __volatile__
	(
		ASM_LWSYNC
		"stw	%1, %0\n\t"
		: "=m" (*p)
		: "r" (v)
		: "memory"
	);
}

static inline void atomic_store_explicit (volatile int *p, int v, int /* memory_order_seq_cst_t */)
{
	__asm__ __volatile__
	(
		ASM_SYNC
		"stw	%1, %0\n\t"
		: "=m" (*p)
		: "r" (v)
		: "memory"
	);
}

/*
 *	native word support
 */

static inline atomic_word atomic_load_explicit (const volatile atomic_word *p, memory_order_relaxed_t)
{
	return *p;
}

static inline atomic_word atomic_load_explicit (const volatile atomic_word *p, memory_order_acquire_t)
{
	atomic_word res;
	__asm__ __volatile__
	(
		"ld		%0, %1\n\t"
		"tdi	0, %0, 0\n\t"
		ASM_ISYNC
		: "=r" (res) : "m" (*p) : "memory"
	);
	return res;
}

static inline atomic_word atomic_load_explicit (const volatile atomic_word *p, int /* memory_order_seq_cst_t */)
{
	atomic_word res;
	__asm__ __volatile__
	(
        ASM_SYNC
		"ld		%0, %1\n\t"
		"tdi	0, %0, 0\n\t"
		ASM_ISYNC
		: "=r" (res) : "m" (*p) : "memory"
	);
	return res;
}

static inline void atomic_store_explicit (volatile atomic_word *p, atomic_word v, memory_order_relaxed_t)
{
	*p = v;
}

static inline void atomic_store_explicit (volatile atomic_word *p, atomic_word v, memory_order_release_t)
{
	__asm__ __volatile__
	(
		ASM_LWSYNC
		"std		%1, %0\n\t"
		: "=m" (*p) : "r" (v) : "memory"
	);
}

static inline void atomic_store_explicit (volatile atomic_word *p, atomic_word v, int /* memory_order_seq_cst_t */)
{
	__asm__ __volatile__
	(
		ASM_SYNC
		"std		%1, %0\n\t"
		: "=m" (*p) : "r" (v) : "memory"
	);
}

#define ATOMIC_XCHG(PRE, POST) \
	atomic_word w; \
	__asm__ __volatile__ \
	( \
		PRE \
	ASM_LABEL(0) \
		"ldarx	%0, 0, %3\n\t" \
		"stdcx. %2, 0, %3\n\t" \
		"bne-   0b\n\t" \
        POST \
		: "=&b" (w), "+m" (*p) \
		: "b" (v), "b" (p) \
		: "cr0", "memory"
	); \
	return w;

static inline atomic_word atomic_exchange_explicit (volatile atomic_word* p, atomic_word v, memory_order_relaxed_t)
{
    ATOMIC_XCHG("", "")
}

static inline atomic_word atomic_exchange_explicit (volatile atomic_word* p, atomic_word v, memory_order_release_t)
{
    ATOMIC_XCHG(ASM_LWSYNC, "")
}

static inline atomic_word atomic_exchange_explicit (volatile atomic_word* p, atomic_word v, memory_order_acquire_t)
{
    ATOMIC_XCHG("", ASM_ISYNC)
}

static inline atomic_word atomic_exchange_explicit (volatile atomic_word* p, atomic_word v, memory_order_release_t)
{
    ATOMIC_XCHG(ASM_LWSYNC, ASM_ISYNC)
}

static inline atomic_word atomic_exchange_explicit (volatile atomic_word* p, atomic_word v, int /* memory_order_seq_cst_t */)
{
    ATOMIC_XCHG(ASM_SYNC, ASM_ISYNC)
}

// atomic_compare_exchange_weak_explicit: can fail spuriously even if *p == *oldval

#define ATOMIC_CMP_XCHG(PRE, POST) \
    atomic_word success; \
	__asm__ __volatile__ \
	( \
		PRE \
		"ldarx	%0, 0, %3\n\t" \
		"eord.	%2, %4, %0\n\t" \
		"bne-	1f\n\t" \
		"stdcx. %5, 0, %3\n\t" \
		POST \
		: "=&b" (*oldval), "+m" (*p), "=&b" (success) \
		: "r" (p), "r" (*oldval), "r" (newval) \
		: "cr0", "memory" \
	); \
	return success;

static inline bool atomic_compare_exchange_weak_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_relaxed_t, memory_order_relaxed_t)
{
    ATOMIC_CMP_XCHG("", ASM_MOVCC0(2) ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_weak_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_release_t, memory_order_relaxed_t)
{
    ATOMIC_CMP_XCHG(ASM_LWSYNC, ASM_MOVCC0(2) ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_weak_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_acquire_t, memory_order_relaxed_t)
{
    ATOMIC_CMP_XCHG("", ASM_MOVCC0(2) ASM_ISYNC ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_weak_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_acq_rel_t, memory_order_relaxed_t)
{
    ATOMIC_CMP_XCHG(ASM_LWSYNC, ASM_MOVCC0(2) ASM_ISYNC ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_weak_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, int /* memory_order_seq_cst_t */, memory_order_relaxed_t)
{
    ATOMIC_CMP_XCHG(ASM_SYNC, ASM_MOVCC0(2) ASM_ISYNC ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_weak_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_release_t, memory_order_release_t)
{
    ATOMIC_CMP_XCHG(ASM_LWSYNC, ASM_MOVCC0(2) ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_weak_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_acquire_t, memory_order_acquire_t)
{
    ATOMIC_CMP_XCHG("", ASM_MOVCC0(2) ASM_LABEL(1) ASM_ISYNC)
}

static inline bool atomic_compare_exchange_weak_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_acq_rel_t, memory_order_acq_rel_t)
{
    ATOMIC_CMP_XCHG(ASM_LWSYNC, ASM_MOVCC0(2) ASM_LABEL(1) ASM_ISYNC)
}

static inline bool atomic_compare_exchange_weak_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, int /* memory_order_seq_cst_t */, int /* memory_order_seq_cst_t */)
{
    ATOMIC_CMP_XCHG(ASM_SYNC, ASM_MOVCC0(2) ASM_LABEL(1) ASM_ISYNC)
}

// *begin-nonstandard-formatting*
#define ATOMIC_OP(PRE, OP, POST) \
    atomic_word res, tmp; \
	__asm__ __volatile__ \
	( \
		PRE \
    ASM_LABEL(0) \
		"ldarx	%0, 0, %3\n\t" \
		OP "    %2, %0, %4\n\t"
		"stdcx. %2, 0, %3\n\t" \
        "bne-   0b\n\t" \
		POST \
		: "=&b" (res), "+m" (*p), "=&b" (tmp) \
		: "r" (p), "r" (v) \
		: "cr0", "memory" \
	); \
	return res;
// *end-nonstandard-formatting*

static inline bool atomic_compare_exchange_strong_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_relaxed_t, memory_order_relaxed_t)
{
    ATOMIC_CMP_XCHG("", ASM_MOV1(2) ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_strong_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_release_t, memory_order_relaxed_t)
{
    ATOMIC_CMP_XCHG(ASM_LWSYNC, ASM_MOV1(2) ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_strong_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_acquire_t, memory_order_relaxed_t)
{
    ATOMIC_CMP_XCHG("", ASM_ISYNC ASM_MOV1(2) ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_strong_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_acq_rel_t, memory_order_relaxed_t)
{
    ATOMIC_CMP_XCHG(ASM_LWSYNC, ASM_ISYNC ASM_MOV1(2) ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_strong_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, int /* memory_order_seq_cst_t */, memory_order_relaxed_t)
{
    ATOMIC_CMP_XCHG(ASM_SYNC, ASM_ISYNC ASM_MOV1(2) ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_strong_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_release_t, memory_order_release_t)
{
    ATOMIC_CMP_XCHG(ASM_LWSYNC, ASM_MOV1(2) ASM_LABEL(1))
}

static inline bool atomic_compare_exchange_strong_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_acquire_t, memory_order_acquire_t)
{
    ATOMIC_CMP_XCHG("", ASM_MOV1(2) ASM_LABEL(1) ASM_ISYNC)
}

static inline bool atomic_compare_exchange_strong_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, memory_order_acq_rel_t, memory_order_acq_rel_t)
{
    ATOMIC_CMP_XCHG(ASM_LWSYNC, ASM_MOV1(2) ASM_LABEL(1) ASM_ISYNC)
}

static inline bool atomic_compare_exchange_strong_explicit (volatile atomic_word* p, atomic_word *oldval, atomic_word newval, int /* memory_order_seq_cst_t */, int /* memory_order_seq_cst_t */)
{
    ATOMIC_CMP_XCHG(ASM_SYNC, ASM_MOV1(2) ASM_LABEL(1) ASM_ISYNC)
}

static inline atomic_word atomic_fetch_add_explicit (volatile atomic_word* p, atomic_word v, memory_order_relaxed_t)
{
    ATOMIC_OP("", "add", "")
}

static inline atomic_word atomic_fetch_add_explicit (volatile atomic_word* p, atomic_word v, memory_order_release_t)
{
    ATOMIC_OP(ASM_LWSYNC, "add", "")
}

static inline atomic_word atomic_fetch_add_explicit (volatile atomic_word* p, atomic_word v, memory_order_acquire_t)
{
    ATOMIC_OP("", "add", ASM_ISYNC)
}

static inline atomic_word atomic_fetch_add_explicit (volatile atomic_word* p, atomic_word v, memory_order_acq_rel_t)
{
    ATOMIC_OP(ASM_LWSYNC, "add", ASM_ISYNC)
}

static inline atomic_word atomic_fetch_add_explicit (volatile atomic_word* p, atomic_word v, int /* memory_order_seq_cst_t */)
{
    ATOMIC_OP(ASM_SYNC, "add", ASM_ISYNC)
}

static inline atomic_word atomic_fetch_sub_explicit (volatile atomic_word* p, atomic_word v, memory_order_relaxed_t)
{
    ATOMIC_OP("", "sub", "")
}

static inline atomic_word atomic_fetch_sub_explicit (volatile atomic_word* p, atomic_word v, memory_order_release_t)
{
    ATOMIC_OP(ASM_LWSYNC, "sub", "")
}

static inline atomic_word atomic_fetch_sub_explicit (volatile atomic_word* p, atomic_word v, memory_order_acquire_t)
{
    ATOMIC_OP("", "sub", ASM_ISYNC)
}

static inline atomic_word atomic_fetch_sub_explicit (volatile atomic_word* p, atomic_word v, memory_order_acq_rel_t)
{
    ATOMIC_OP(ASM_LWSYNC, "sub", ASM_ISYNC)
}

static inline atomic_word atomic_fetch_sub_explicit (volatile atomic_word* p, atomic_word v, int /* memory_order_seq_cst_t */)
{
    ATOMIC_OP(ASM_SYNC, "sub", ASM_ISYNC)
}

#undef ATOMIC_OP
#define ATOMIC_OP(PRE, OP, POST) \
    int res, tmp; \
	__asm__ __volatile__ \
	( \
		PRE \
    ASM_LABEL(0) \
		"lwarx	%0, 0, %3\n\t" \
		OP "    %2, %0, %4\n\t" \
		"stwcx. %2, 0, %3\n\t" \
        "bne-   0b\n\t" \
		POST \
		: "=&b" (res), "+m" (*p), "=&b" (tmp) \
		: "r" (p), "r" (v) \
		: "cr0", "memory" \
	); \
	return res;

static inline int atomic_fetch_add_explicit (volatile int* p, int v, memory_order_relaxed_t)
{
    ATOMIC_OP("", "add", "")
}

static inline int atomic_fetch_add_explicit (volatile int* p, int v, memory_order_release_t)
{
    ATOMIC_OP(ASM_LWSYNC, "add", "")
}

static inline int atomic_fetch_add_explicit (volatile int* p, int v, memory_order_acquire_t)
{
    ATOMIC_OP("", "add", ASM_ISYNC)
}

static inline int atomic_fetch_add_explicit (volatile int* p, int v, memory_order_acq_rel_t)
{
    ATOMIC_OP(ASM_LWSYNC, "add", ASM_ISYNC)
}

static inline int atomic_fetch_add_explicit (volatile int* p, int v, int /* memory_order_seq_cst_t */)
{
    ATOMIC_OP(ASM_SYNC, "add", ASM_ISYNC)
}

static inline int atomic_fetch_sub_explicit (volatile int* p, int v, memory_order_relaxed_t)
{
    ATOMIC_OP("", "sub", "")
}

static inline int atomic_fetch_sub_explicit (volatile int* p, int v, memory_order_release_t)
{
    ATOMIC_OP(ASM_LWSYNC, "sub", "")
}

static inline int atomic_fetch_sub_explicit (volatile int* p, int v, memory_order_acquire_t)
{
    ATOMIC_OP("", "sub", ASM_ISYNC)
}

static inline int atomic_fetch_sub_explicit (volatile int* p, int v, memory_order_acq_rel_t)
{
    ATOMIC_OP(ASM_LWSYNC, "sub", ASM_ISYNC)
}

static inline int atomic_fetch_sub_explicit (volatile int* p, int v, int /* memory_order_seq_cst_t */)
{
    ATOMIC_OP(ASM_SYNC, "sub", ASM_ISYNC)
}

/*
 *  extensions
 */

static inline void atomic_retain (volatile int* p)
{
    atomic_fetch_add_explicit (p, 1, memory_order_relaxed);
}

static inline bool atomic_release (volatile int* p)
{
	bool res = atomic_fetch_sub_explicit (p, 1, memory_order_release) == 1;
    if (res)
	{
		atomic_thread_fence (memory_order_acquire);
	}
	return res;
}
