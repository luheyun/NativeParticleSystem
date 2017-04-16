#ifndef RAND_H
#define RAND_H
/*
Some random generator timings:
MacBook Pro w/ Core 2 Duo 2.4GHz. Times are for gcc 4.0.1 (OS X 10.6.2) / VS2008 SP1 (Win XP SP3),
in milliseconds for this loop (4915200 calls):

 for (int j = 0; j < 100; ++j)
   for (int i = 0; i < 128*128*3; ++i)
     data[i] = (rnd.get() & 0x3) << 6;

                  gcc   vs2008    Size
C's rand():       57.0  109.3 ms     1
Mersenne Twister: 56.0   37.4 ms  2500
Unity 2.x LCG:    11.1    9.2 ms     4
Xorshift 128:     15.0   17.8 ms    16
Xorshift 32:      20.6   10.7 ms     4
WELL 512:         43.6   55.1 ms    68
*/

struct RandState
{
	UInt32 x, y, z, w;
};


// Xorshift 128 implementation
// Xorshift paper: http://www.jstatsoft.org/v08/i14/paper
// Wikipedia: http://en.wikipedia.org/wiki/Xorshift
class Rand
{
public:
	Rand (UInt32 seed = 0)
	{
		SetSeed (seed);
	}

	UInt32 Get ()
	{
		UInt32 t;
		t = state.x ^ (state.x << 11);
		state.x = state.y; state.y = state.z; state.z = state.w;
		return state.w = (state.w ^ (state.w >> 19)) ^ (t ^ (t >> 8));
	}

	UInt64 Get64 ()
	{
		// Xorshift random number generators developed by George Marsaglia.
		// Reference article: Marsaglia, George. "Xorshift RNGs". Journal of Statistical Software 8 (14), 2003, July
		UInt64 t  = Get ();
		t ^= t >> 12;
		t ^= t << 25;
		t ^= t >> 27;
		return t * 2685821657736338717LL;
	}

	inline static float GetFloatFromInt (UInt32 value)
	{
		// take 23 bits of integer, and divide by 2^23-1
		return float(value & 0x007FFFFF) * (1.0f / 8388607.0f);
	}

	inline static UInt8 GetByteFromInt (UInt32 value)
	{
		// take the most significant byte from the 23-bit value
		return UInt8(value >> (23 - 8));
	}

	// random number between 0.0 and 1.0
	float GetFloat ()
	{
		return GetFloatFromInt (Get ());
	}

	// random number between -1.0 and 1.0
	float GetSignedFloat ()
	{
		return GetFloat() * 2.0f - 1.0f;
	}

	void SetSeed (UInt32 seed)
	{
		// std::mt19937::initialization_multiplier = 1812433253U
		state.x = seed;
		state.y = state.x * 1812433253U + 1;
		state.z = state.y * 1812433253U + 1;
		state.w = state.z * 1812433253U + 1;
	}

	UInt32 GetSeed () const { return state.x; }

	RandState GetState () const { return state; }
	void SetState (RandState value) { state = value; }

private:
	RandState state;	// RNG state (128 bit)
};

Rand& GetScriptingRand();

#endif
