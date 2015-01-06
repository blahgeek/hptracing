/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-06
*/

#include <iostream>
#include "./triangle.h"

#include <Eigen/Geometry>
#include <Eigen/Dense>

using namespace hp;

Vec Triangle::getNormal(const Vec & point) const {
    Vec ret = dx.cross(dy);
    ret.normalize();
    return ret;
}

Number Triangle::do_intersect(const Vec & start_p, const Vec & dir) const {
    Eigen::Matrix<Number, 3, 3> A;
    A << dx, dy, -dir;
    Vec result = A.fullPivLu().solve(start_p);
    if(result[0] >= 0 && result[0] <= 1 &&
       result[1] >= 0 && result[1] <= 1)
        return result[2]; // may < 0, which means no intersection
    return -1;
}
