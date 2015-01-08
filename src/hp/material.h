/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-08
*/

#ifndef __hp_material_h__
#define __hp_material_h__ value

#include "./common.h"

using hp::Number;
using hp::Color;

namespace hp {

class Material {
public:
    Color ambient = {0, 0, 0}; // Ka, light
    Color diffuse = {0, 0, 0}; // Kd
    Color specular = {0, 0, 0}; // Ks
    // Tf(transmission filter) is ignored
    // Ke(emission) is ignored
    // Ns(specular exp) is ignored
    // int specular_exp = 1; // Ns
    /**
     * Ni (optical density) (index of refraction)
     */
    Number optical_density = 1.5;
    /**
     * A factor of 1.0 is fully opaque.
     * A factor of 0.0 is fully dissolved (completely transparent)
     */
    Number dissolve = 1.0;

    // illum is ignored

    // std::string diffuse_map; // map_Kd
    // ambient map, specular map is ignored

};

}

#endif
