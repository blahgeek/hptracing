/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-06
*/

#ifndef __hp_geometry_sphere_h__
#define __hp_geometry_sphere_h__ value

#include "./base.h"

using hp::Vec;
using hp::Geometry;

namespace hp {

class Sphere : public Geometry {

private:
    Vec center;
    Number radius;

public:
    Sphere(Vec center, Number radius): center(center), radius(radius) {}

    Vec getNormal(const Vec & point) const override;

protected:
    std::pair<bool, Vec> do_intersect(const Vec & start_p, const Vec & dir) const override;

};

}

#endif
