/* 
* @Author: BlahGeek
* @Date:   2015-01-05
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-05
*/

#ifndef __hp_common_h__
#define __hp_common_h__ value

#include <memory>
#include <string>

#include <cstddef>
#include <cstdarg>

extern bool g_hp_log_enable;

#define likely(v)   __builtin_expect(bool(v), 1)
#define unlikely(v)   __builtin_expect(bool(v), 0)

#define LOCK_GUARD(mtx) \
	std::lock_guard<decltype(mtx)> __lock_guard_##mtx(mtx)

#define hp_log(fmt...) \
	__hp_log__(__FILE__, __func__, __LINE__, fmt)

/*!
 * \brief printf-like std::string constructor
 */
std::string svsprintf(const char *fmt, va_list ap);

std::string ssprintf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));


#define hp_assert(expr, msg...) \
    do { \
        if (unlikely(!(expr))) \
            __usql_assert_fail__(__FILE__, __LINE__, \
                    __PRETTY_FUNCTION__, # expr, ##msg); \
    } while(0)

void __hp_assert_fail__(
        const char *file, int line, const char *func,
        const char *expr, const char *msg_fmt = nullptr, ...)
    __attribute__((format(printf, 5, 6), noreturn));

void __hp_log__(const char *file, const char *func, int line,
		const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

#endif
