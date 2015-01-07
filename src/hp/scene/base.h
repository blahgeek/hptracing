/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-07
*/

#ifndef __hp_scene_base_h__
#define __hp_scene_base_h__ value

#include "../common.h"
#include "../geometry/base.h"
#include "../material.h"

#include "../../objloader/tiny_obj_loader.h"

#include <vector>
#include <map>
#include <memory>
#include <tuple>

using hp::Geometry;
using hp::Material;
using hp::Vec;

namespace hp {

class Scene {
private:
    std::vector<Material> materials;

protected:
    std::vector<std::pair<std::unique_ptr<Geometry>, int>> geometries;

public:
    Scene(int num_geo, int num_mat);
    virtual ~Scene(){}
    
    void setMaterial(int n, Material mat);
    virtual void setGeometry(int n, std::unique_ptr<Geometry> && geo, int mat_id);

    virtual std::tuple<Number, Geometry *, Material *> 
        intersect(const Vec & start_p, const Vec & dir);

};
    
}

#endif
