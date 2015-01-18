/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-18
*/

#ifndef __hp_scene_cl_base_h__
#define __hp_scene_cl_base_h__ value

#include "../cl_src/types.h"
#include "../common.h"
#include <vector>
#include <string>

namespace hp {

    class Scene {
    protected:
        hp::Number total_light_val = 0;
        std::map<hp::Number, int> lights_map;
        // computes lights
        void registerGeometry(cl_int4 triangle);
        void finishRegister();
    public:
        std::vector<hp::Material> materials;

        std::vector<cl_float3> points;
        std::vector<cl_float3> normals; // points normals

        std::vector<cl_int4> lights;
        std::vector<cl_int4> geometries;

        Scene(std::string filename);

    };

}


#endif
