/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-06
*/

#include "./sphere.h"

using namespace hp;

Vec Sphere::getNormal(const Vec & point) const {
    Vec ret = point - center;
    ret.normalize();
    return ret;
}

std::pair<bool, Vec> Sphere::do_intersect(const Vec & start_p, const Vec & dir) const {
    Vec p = start_p - center;
    Vec x = (-dir.dot(p)) * dir;
    Vec y = p + x;
    auto y_norm = y.norm();
    if(y_norm > radius)
        return {false, start_p};
    auto x_dot = x.dot(dir);

    auto s = std::sqrt(radius * radius - y_norm * y_norm);
    auto t_norm = x_dot - s;
    if(t_norm < 0)
        t_norm += 2 * s;
    if(t_norm < 0)
        return {false, start_p};
    return {true, dir * t_norm + start_p};
}
