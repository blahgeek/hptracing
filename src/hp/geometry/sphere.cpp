/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-08
*/

#include "./sphere.h"

using namespace hp;

Vec Sphere::getNormal(const Vec & point) const {
    Vec ret = point - center;
    ret.normalize();
    return ret;
}

Number Sphere::do_intersect(const Vec & start_p, const Vec & dir) const {
    Vec p = start_p - center;
    Vec x = (-dir.dot(p)) * dir;
    Vec y = p + x;
    auto y_norm = y.norm();
    if(y_norm > radius)
        return -1;
    auto x_dot = x.dot(dir);

    auto s = std::sqrt(radius * radius - y_norm * y_norm);
    auto t_norm = x_dot - s;
    if(t_norm < 0)
        t_norm += 2 * s;
    return t_norm; // may < 0, which means no intersection
}

Number Sphere::getSurfaceArea() const {
    return 4 * PI * radius * radius;
}

Vec Sphere::randomSurfacePoint() const {
    Vec p(RAND_F(), RAND_F(), RAND_F()); p.normalize();
    return center + radius * p;
}
