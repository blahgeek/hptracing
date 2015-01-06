/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-06
*/

#include <iostream>
#include <cmath>
#include <limits>
#include "./base.h"

using namespace hp;

Vec Geometry::getReflection(const Vec & in_dir,
                            const Vec & normal_dir) {
    auto dot = in_dir.dot(normal_dir);
    Vec projection = dot * normal_dir;
    return in_dir - 2 * projection;
}

Vec Geometry::getRefraction(const Vec & in_dir,
                            const Vec & normal_dir,
                            Number k) {
    Number cos_alpha = in_dir.dot(-normal_dir);
    Number reverse = (cos_alpha < 0) ? -1 : 1;
    Number alpha = std::acos(cos_alpha);
    Vec p = cos_alpha * normal_dir;
    Vec q = in_dir + p;
    Number sin_beta = std::sin(alpha) * std::pow(k, reverse);
    Number beta = std::asin(sin_beta);

    if(q.norm() > 0) q.normalize();
    return -reverse * std::cos(beta) * normal_dir +
            sin_beta * q;
}

Number Geometry::intersect(const Vec & start_p, const Vec & dir) const {
    Vec start = start_p + 1e-5 * dir; // epsilon
    return this->do_intersect(start, dir);
}
