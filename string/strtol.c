#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_BASE		36
#define DEFAULT_BASE	10

#define BINARY_BASE		2
#define OCTAL_BASE		8
#define DECIMAL_BASE	10
#define HEXADECIMAL_BASE	16

#define DEBUG_PRINT		0

static void print_null(char *format, ...) {};
#if DEBUG_PRINT
#define debug	printf
#else
#define debug	print_null
#endif

static bool inline isdigital(char ch)
{
	return (((ch >= '0') && (ch <= '9'))) ? true : false;
}

static char inline mytoupper(char ch)
{
	return ((ch >= 'a' && ch <= 'z')) ? (ch - 'a' + 'A') : ch;
}

static char inline mytolower(char ch)
{
	return ((ch >= 'A' && ch <= 'Z')) ? (ch - 'A' + 'a') : ch;
}

static bool inline iscaptal(char ch)
{
	return ((mytolower(ch) >= 'a') && (mytolower(ch) <= 'z')) ? true : false;
}

static bool inline isbroaddigital(char ch)
{
	return (isdigital(ch) || iscaptal(ch)) ? true : false;
}

static int inline todecimal(char ch)
{
	return isdigital(ch) ? (ch - '0') : (mytolower(ch) - 'a' + 10);
}

static bool inline isbroadlegdigital(char ch, int base)
{
	if(isdigital(ch) || iscaptal(ch)) 
		return (todecimal(ch) < base) ? true : false;

	return false;
}

static bool inline issign(char ch)
{
	return ((ch == '-') || (ch == '+')) ? true : false;
}

static bool inline islegalchar(char ch, int base)
{
	if(isbroadlegdigital(ch, base) || issign(ch))
		return true;

	return false;
}

static int strlen(const char *nptr)
{
	int cnt = 0;
	char const *p = nptr;

	if(!nptr) return -1;

	while(*p++) cnt++;

	return cnt;
}

int strtol(const char *nptr, long *pval, int nlen, int base)
{
	int i;
	long sum = 0;
	bool negaflag = false;
	char const *p = nptr;

	if(!nptr || !pval) return -1;
	if(base < 0 || base > MAX_BASE || base == 1) {
		debug("error(%d):Invalid arguments\n", __LINE__);
		return -1;
	}

	//skip the space
	for(; *p == ' '; p++);
	if(!islegalchar(*p, MAX_BASE)) {
		debug("error(%d):\'%c\' is a illegal char\n", __LINE__, *p);
		return -1;
	}
	if(issign(*p)) {
		negaflag = (*p == '-') ? true : false;
		
		if(!isbroaddigital(*++p)) {
			debug("error(%d):\'%c\' is a illegal char\n", __LINE__, *p);
			return -1;
		}
	}

	//skip '0x'
	if(!base || (base == HEXADECIMAL_BASE)) {
		if((*p == '0') && (mytolower(*(p + 1)) == 'x')) {
			p += 2;
			base = HEXADECIMAL_BASE;
		}
	}
	//skip '0'
	if(!base || (base == OCTAL_BASE)) {
		if(*p == '0') {
			p += 1;
			base = OCTAL_BASE;
		}
	}
	if(!base)
		base = DECIMAL_BASE;

	//calculate the real nlen and base
	nlen = (nlen > 0) ? (nlen - (p - nptr)) : strlen(p);
	if(!isbroadlegdigital(*p, base)) {
		debug("error(%d):\'%c\' is a illegal char\n", __LINE__, *p);
		return -1;
	}

	for(i = 0; i < nlen; i++, p++) {
		if(isbroadlegdigital(*p, base))
			sum = sum * base + todecimal(*p);
		else
			break;
		//边界判断
		if((!negaflag && (sum < 0)) || (negaflag && (-sum > 0))) {
			debug("error(%d):Beyond the border\n", __LINE__);
			return -1;
		}
	}
	
	*pval =  negaflag ? -sum : sum;

	return 0;
}
