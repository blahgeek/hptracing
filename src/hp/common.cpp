/* 
* @Author: BlahGeek
* @Date:   2015-01-05
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-12-25
*/

#include "./common.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <sys/time.h>

#include <unistd.h>
#ifndef _GNU_SOURCE
#include <libgen.h>
#endif

using namespace hp;

static uint64_t GetTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

TickTock::TickTock() {
    this->t = GetTimeStamp();
}

void TickTock::timeit(const char * msg_fmt, ...) {
    auto new_t = GetTimeStamp();
    auto delta = new_t - this->t;
    this->t = new_t;

    fprintf(stderr, "[TickTock] %lldms: ", delta / 1000);
    va_list ap;
    va_start(ap, msg_fmt);
    vfprintf(stderr, msg_fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}


bool hp::g_log_enable = true;

void hp::__hp_log__(const char *file, const char *func, int line,
		const char *fmt, ...) {

    if(!g_log_enable)
        return;

#define TIME_FMT	"[%s %s@%s:%d]"
	static std::mutex mtx;

	static const char *time_fmt = nullptr;
	if (!time_fmt) {
		if (isatty(fileno(stderr)))
			time_fmt = "\033[33m" TIME_FMT "\033[0m ";
		else
			time_fmt = TIME_FMT " ";
	}
	time_t cur_time;
	time(&cur_time);
	char timestr[64];
	strftime(timestr, sizeof(timestr), "%H:%M:%S",
			localtime(&cur_time));

	{
		LOCK_GUARD(mtx);
        #ifdef _GNU_SOURCE
		fprintf(stderr, time_fmt, timestr, func, basename(strdupa(file)), line);
        #else
        // strdupa is only present on GNU GCC
        char * file_dup = strdup(file);
        fprintf(stderr, time_fmt, timestr, func, basename(file_dup), line);
        free(file_dup);
        #endif

		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fputc('\n', stderr);
	}

}

void hp::__hp_assert_fail__(
        const char *file, int line, const char *func,
        const char *expr, const char *msg_fmt, ...) {
    std::string msg;
    if (msg_fmt) {
        va_list ap;
        va_start(ap, msg_fmt);
        msg = "\nextra message: ";
        msg.append(svsprintf(msg_fmt, ap));
        va_end(ap);
    }
    fprintf(stderr,
            "assertion `%s' failed at %s:%d: %s%s\n",
            expr, file, line, func, msg.c_str());
    __builtin_trap();
}

std::string hp::svsprintf(const char *fmt, va_list ap_orig) {
	int size = 100;     /* Guess we need no more than 100 bytes */
	char *p;

	if ((p = (char*)malloc(size)) == nullptr)
		goto err;

	for (; ;) {
		va_list ap;
		va_copy(ap, ap_orig);
		int n = vsnprintf(p, size, fmt, ap);
		va_end(ap);

		if (n < 0)
			goto err;

		if (n < size) {
			std::string rst(p);
			free(p);
			return rst;
		}

		size = n + 1;

		char *np = (char*)realloc(p, size);
		if (!np) {
			free(p);
			goto err;
		} else 
			p = np;
	}

err:
	fprintf(stderr, "could not allocate memory for svsprintf\n");
    __builtin_trap();
}

std::string hp::ssprintf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	auto rst = svsprintf(fmt, ap);
	va_end(ap);
	return rst;
}

// vim: syntax=cpp.doxygen foldmethod=marker foldmarker=f{{{,f}}}

