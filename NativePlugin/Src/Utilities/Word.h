#pragma once

// $TODO#scobi(9-dec-14) fix name - this only works on A-Z and is not intended for even a latin char set, yet it has the same name as a std function and no guidance given on when it should be used.
inline char ToLower(char c) { return (c >= 'A' && c <= 'Z') ? (c + 'a' - 'A') : c; }
inline char ToUpper(char c) { return (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c; }
inline unsigned char ToLower(unsigned char c) { return (c >= 'A' && c <= 'Z') ? (c + 'a' - 'A') : c; }
inline unsigned char ToUpper(unsigned char c) { return (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c; }

template<typename TString>
TString ToUpper(const TString& input)
{
	TString s = input;
	for (typename TString::iterator i = s.begin(); i != s.end(); i++)
		*i = ToUpper(*i);
	return s;
}