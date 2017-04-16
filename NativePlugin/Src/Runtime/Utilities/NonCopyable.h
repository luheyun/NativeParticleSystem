#ifndef NON_COPYABLE_H
#define NON_COPYABLE_H

#ifndef EXPORT_COREMODULE
#define EXPORT_COREMODULE
#endif

class NonCopyable
{
public:
	NonCopyable() {}

private:
	NonCopyable(const NonCopyable&);
	NonCopyable& operator=(const NonCopyable&);
};

#endif
