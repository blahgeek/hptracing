/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-08
*/

#ifndef __hp_geometry_triangle_h__
#define __hp_geometry_triangle_h__ value

#include "./base.h"

using hp::Vec;
using hp::Geometry;

namespace hp {

class Triangle: public Geometry {

private:
    Vec p0;
    Vec dx, dy;

public:
    Triangle(const Vec & a, const Vec & b, const Vec & c):
        p0(a), dx(b-a), dy(c-a) {}
    Vec getNormal(const Vec & point) const override;
    Number getSurfaceArea() const override;
    Vec randomSurfacePoint() const override;

protected:
    Number do_intersect(const Vec & start_p, const Vec & dir) const override;

};

}

#endif
