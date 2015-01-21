/* 
* @Author: BlahGeek
* @Date:   2015-01-05
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-21
*/

#ifndef __hp_common_h__
#define __hp_common_h__ value

#include <memory>
#include <string>

#include <cstddef>
#include <cstdarg>

#include <Eigen/Core>
#include "../json/json.h"

#define PI 3.141592653589

#define RAND_F() (Number(std::rand()) / Number(RAND_MAX) - 0.5)
#define ASSIGN_F3(X, Y) \
    do { \
        (X).s[0] = (Y)[0]; \
        (X).s[1] = (Y)[1]; \
        (X).s[2] = (Y)[2]; \
        (X).s[3] = 0; \
    } while(0)
#define ASSIGN_V3(X, Y) \
    do { \
        (X)[0] = (Y).s[0]; \
        (X)[1] = (Y).s[1]; \
        (X)[2] = (Y).s[2]; \
    } while(0)

namespace hp {

using Number = float;
using Vec = Eigen::Matrix<Number, 3, 1>;
using Color = Eigen::Matrix<Number, 3, 1>;

extern bool g_log_enable;

#define likely(v)   __builtin_expect(bool(v), 1)
#define unlikely(v)   __builtin_expect(bool(v), 0)

#define LOCK_GUARD(mtx) \
	std::lock_guard<decltype(mtx)> __lock_guard_##mtx(mtx)

#define hp_log(fmt...) \
	__hp_log__(__FILE__, __func__, __LINE__, fmt)

class TickTock {
private:
    uint64_t t = 0;
public:
    TickTock();
    void timeit(const char *msg_fmt = nullptr, ...)
        __attribute__((format(printf, 2, 3)));
};

/*!
 * \brief printf-like std::string constructor
 */
std::string svsprintf(const char *fmt, va_list ap);

std::string ssprintf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));


#define hp_assert(expr, msg...) \
    do { \
        if (unlikely(!(expr))) \
            __hp_assert_fail__(__FILE__, __LINE__, \
                    __PRETTY_FUNCTION__, # expr, ##msg); \
    } while(0)

void __hp_assert_fail__(
        const char *file, int line, const char *func,
        const char *expr, const char *msg_fmt = nullptr, ...)
    __attribute__((format(printf, 5, 6), noreturn));

void __hp_log__(const char *file, const char *func, int line,
		const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));
    
}

#endif
