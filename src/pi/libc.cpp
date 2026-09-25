// The few C library functions HarfBuzz references that Circle doesn't
// provide. None of them are on the shaping path the app uses (they serve
// buffer serialisation and AAT fonts), but they must link and should work.
#include <circle/string.h>
#include <circle/util.h>
#include <stdarg.h>

extern "C" {

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	CString s;
	s.FormatV(fmt, args);
	va_end(args);
	size_t len = s.GetLength();
	if (size) {
		size_t n = len < size - 1 ? len : size - 1;
		memcpy(buf, (const char *)s, n);
		buf[n] = 0;
	}
	return (int)len;
}

long strtol(const char *s, char **end, int base)
{
	while (*s == ' ' || *s == '\t')
		s++;
	bool neg = *s == '-';
	if (*s == '-' || *s == '+')
		s++;
	unsigned long v = strtoul(s, end, base);
	return neg ? -(long)v : (long)v;
}

// Shell sort: small, no recursion, fine for the short arrays involved.
void qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *))
{
	char *a = (char *)base;
	for (size_t gap = n / 2; gap > 0; gap /= 2)
		for (size_t i = gap; i < n; i++)
			for (size_t j = i; j >= gap && cmp(a + (j - gap) * size, a + j * size) > 0; j -= gap)
				for (char *x = a + (j - gap) * size, *y = a + j * size, *e = y + size; y < e; x++, y++) {
					char t = *x;
					*x = *y;
					*y = t;
				}
}

} // extern "C"
