/* 
* @Author: BlahGeek
* @Date:   2015-01-13
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-13
*/

#ifndef __hp_uniform_box_h__
#define __hp_uniform_box_h__ value

#include "./base.h"

namespace hp{
namespace cl{

    class UniformBox: public Scene {
    private:
        cl_float3 getMin();
        cl_float3 getMax();

        int getIndex(int i, int j, int k) {
            return i * split_n.s[1] * split_n.s[2] + j * split_n.s[2] + k;
        }
    public:
        struct Node {
            std::vector<cl_int> geometries;
            cl_int neighbor[6]; // x-, x+, y-, y+, z-, z+   -1 for null
        };
        cl_int3 split_n;

        cl_float3 box_size;
        cl_float3 box_start;
        cl_float3 box_end;
        std::vector<Node> data;

    public:
        UniformBox(std::string filename, 
                   int splitx = 100, int splity = 100, int splitz = 100);

    };

}
}

#endif
