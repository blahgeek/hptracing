/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-08
*/

#ifndef __hp_geometry_base_h__
#define __hp_geometry_base_h__ value

#include "../common.h"

using hp::Vec;

namespace hp {

class Geometry {
public:
    /**
     * Compute intersect of a ray
     * @param  start_p Start point of ray (may inside/on this object)
     * @param  dir     Direction of ray, must be normalized
     * @return         Distance from start point to intersection point
     *                 < 0 if no intersection
     */
    Number intersect(const Vec & start_p, const Vec & dir) const;

    /**
     * Compute normal vector at this point 
     * @param  point Point, may not be exactly on the surface
     * @return       vector
     */
    virtual Vec getNormal(const Vec & point) const = 0;

    virtual Number getSurfaceArea() const = 0;

    virtual Vec randomSurfacePoint() const = 0;

    /**
     * get reflection
     * @param  in_dir     input direction
     * @param  normal_dir normal direction
     * @return            output direction
     */
    static Vec getReflection(const Vec & in_dir, const Vec & normal_dir);

    /**
     * get refraction
     * @param  k          refraction constant of object
     */
    static Vec getRefraction(const Vec & in_dir, const Vec & normal_dir, Number k);

    virtual ~Geometry() {}

protected:
    virtual Number do_intersect(const Vec & start_p, const Vec & dir) const = 0;

};

}

#endif
