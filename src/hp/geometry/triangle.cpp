/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-08
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
    Vec b = start_p - p0;
    A << dx, dy, dir;
    Vec result = A.fullPivLu().solve(b);
    // auto error = (A * result - b).norm() / b.norm();
    // std::cerr << "error: " << error << std::endl;
    // std::cerr << "result: " << result[0] << " " << result[1] << " " << result[2] << std::endl;
    if(result[0] >= 0 && result[0] <= 1 &&
       result[1] >= 0 && result[1] <= 1 && 
       result[0] + result[1] < 1.0 &&
       result[2] < 0)
        return -result[2];
    return -1;
}

Number Triangle::getSurfaceArea() const {
    Number dot = dx.dot(dy);
    Number cosine = dx.normalized().dot(dy.normalized());
    return dot / cosine * std::sqrt(1 - cosine * cosine);
}

Vec Triangle::randomSurfacePoint() const {
    Number randx = RAND_F() + 0.5;
    Number randy = RAND_F() + 0.5;
    if(randx + randy > 1) {
        randx = 1 - randx;
        randy = 1- randy;
    }
    Vec ret = p0 + dx * randx + dy * randy;
    return ret;
}
