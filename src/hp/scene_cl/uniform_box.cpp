/* 
* @Author: BlahGeek
* @Date:   2015-01-13
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-13
*/

#include <iostream>
#include "./uniform_box.h"
#include <algorithm>

using namespace hp;

cl_float3 cl::UniformBox::getMin() {
    cl_float3 ret;
    for(int i = 0 ; i < 3 ; i += 1) {
        // loop [x, y, z]
        auto it = std::min_element(this->points.begin(),
                                   this->points.end(),
                                   [&](const cl_float3 & a, const cl_float3 & b) {
                                        return a.s[i] < b.s[i];
                                   });
        ret.s[i] = it->s[i];
    }
    return ret;
}

cl_float3 cl::UniformBox::getMax() {
    cl_float3 ret;
    for(int i = 0 ; i < 3 ; i += 1) {
        // loop [x, y, z]
        auto it = std::max_element(this->points.begin(),
                                   this->points.end(),
                                   [&](const cl_float3 & a, const cl_float3 & b) {
                                        return a.s[i] < b.s[i];
                                   });
        ret.s[i] = it->s[i];
    }
    return ret;
}

cl::UniformBox::UniformBox(std::string filename, int splitx, int splity, int splitz): 
        cl::Scene(filename) {
    split_n.s[0] = splitx; split_n.s[1] = splity; split_n.s[2] = splitz;

    box_start = this->getMin();
    box_end = this->getMax();
    for(int i = 0 ; i < 3 ; i += 1) {
        box_start.s[i] -= 0.42f;
        box_end.s[i] += 0.42f;
    }

    for(int i = 0 ; i < 3 ; i += 1)
        box_size.s[i] = (box_end.s[i] - box_start.s[i]) / float(split_n.s[i]);

    hp_log("Constructing Uniform Box from (%f %f %f) to (%f %f %f)",
           box_start.s[0], box_start.s[1], box_start.s[2],
           box_end.s[0], box_end.s[1], box_end.s[2]);
    hp_log("Spliting into (%d %d %d)", splitx, splity, splitz);

    data.resize(splitx * splity * splitz);

    for(int i = 0 ; i < splitx ; i += 1) {
        for(int j = 0 ; j < splity ; j += 1) {
            for(int k = 0 ; k < splitz ; k += 1) {
                auto & target = data[getIndex(i, j, k)];

                for(int x = 0 ; x < 6 ; x += 1)
                    target.neighbor[x] = -1;

                if(i != 0) target.neighbor[0] = getIndex(i-1, j, k);
                if(i < splitx - 1) target.neighbor[1] = getIndex(i+1, j, k);
                if(j != 0) target.neighbor[2] = getIndex(i, j-1, k);
                if(j < splity - 1) target.neighbor[3] = getIndex(i, j+1, k);
                if(k != 0) target.neighbor[4] = getIndex(i, j, k-1);
                if(k < splitz - 1) target.neighbor[5] = getIndex(i, j, k+1);
            }
        }
    }

    for(int geo_index = 0 ; geo_index < this->geometries.size() ; geo_index += 1) {
        auto & geo = this->geometries[geo_index];
        int boundary[3][2];
        for(int d = 0 ; d < 3 ; d += 1) {
            // loop [x, y, z]
            cl_float vals[3] = {
                this->points[geo.s[0]].s[d],
                this->points[geo.s[1]].s[d],
                this->points[geo.s[2]].s[d],
            };
            boundary[d][0] = std::floor((*std::min_element(vals, vals+3) - box_start.s[d]) / box_size.s[d]);
            boundary[d][1] = 1+std::floor((*std::max_element(vals, vals+3) - box_start.s[d]) / box_size.s[d]);
        }
        for(int i = boundary[0][0] ; i < boundary[0][1] ; i += 1) {
            for(int j = boundary[1][0] ; j < boundary[1][1] ; j += 1) {
                for(int k = boundary[2][0] ; k < boundary[2][1] ; k += 1) {
                    data[getIndex(i, j, k)].geometries.push_back(geo_index);
                }
            }
        }
    }

    // int empty_count = 0;

    // for(int i = 0 ; i < splitx ; i += 1) {
    //     for(int j = 0 ; j < splity ; j += 1) {
    //         for(int k = 0 ; k < splitz ; k += 1) {
    //             auto & target = data[getIndex(i, j, k)];
    //             if(target.geometries.size() > 0) 
    //                 continue;

    //             empty_count += 1;

    //             for(int x = 0 ; x < 6 ; x += 1) {
    //                 int y = (x % 2 == 0) ? x + 1 : x - 1;
    //                 if(target.neighbor[x] >= 0)
    //                     data[target.neighbor[x]].neighbor[y] = target.neighbor[y];
    //             }
    //         }
    //     }
    // }

    // hp_log("Empty box: %d", empty_count);


}
