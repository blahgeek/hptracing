/* 
* @Author: BlahGeek
* @Date:   2015-01-14
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-20
*/

#ifndef __hp_scene_cl_kdtree_h__
#define __hp_scene_cl_kdtree_h__ value

#include "./base.h"

namespace hp {

    class KDTree: public hp::Scene {

        class Node {
        public:
            // global values
            std::vector<cl_float3> & points;
            std::vector<cl_int4> & geometries;

            // get min/max value for one triangle in one dimension
            std::pair<cl_float, cl_float> triangleMinMax(cl_int4 geo, int dimension);

        public:
            Node(std::vector<cl_float3> & points, std::vector<cl_int4> & geometries):
                points(points), geometries(geometries) {}
            // if left == null && right == null -> this is leaf
            std::unique_ptr<Node> left = nullptr;
            std::unique_ptr<Node> right = nullptr;
            // if parent == null -> this is root
            Node * parent = nullptr;

            cl_float3 box_start;
            cl_float3 box_end;
            std::vector<cl_int> geo_indexes;
            // std::vector<cl_int4> geos;

            std::vector<cl_float> min_vals[3]; // sorted
            std::vector<cl_float> max_vals[3];
            void calcMinMaxVals();
            void setBoxSize();
            // return split position and cost value
            std::pair<cl_float, cl_float> findBestSplit(int dimension);
            void split();

            void removeEmptyNode();

            int debugPrint(int depth=0, int id = 0);

        };

    public:
        std::unique_ptr<Node> root = nullptr;
        KDTree(std::string filename);
        std::pair<std::vector<KDTreeNodeHeader>, std::vector<cl_int>>
            getData();
    };

}

#endif
