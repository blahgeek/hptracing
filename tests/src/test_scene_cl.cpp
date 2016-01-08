/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-12-25
*/

#include <iostream>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <algorithm>
#include "./hp/geometry/triangle.h"
#include "./hp/scene/base.h"
#include "./hp/scene/kdtree.h"

using namespace std;

TEST(SceneCLTest, load) {
    auto scene = std::make_unique<hp::Scene>(std::string("obj/cornell_box.obj"));
    EXPECT_EQ(scene->lights.size(), 256);
    auto half_count = std::count_if(scene->lights.begin(), scene->lights.end(),
                                   [](const cl_int4 & x)->bool {
                                      return x.s[1] == 14;
                                   });
    EXPECT_EQ(half_count, 127);
}

TEST(SceneCLTest, kdtree_load) {
    auto scene = std::make_unique<hp::KDTree>(std::string("obj/cornell_box.obj"));
    EXPECT_TRUE(bool(scene));
    scene->getData();
}

#define CL_FLOAT3_TO_VEC(X) \
    hp::Vec(X.s[0], X.s[1], X.s[2])

static hp::Number triangle_intersect(hp::Vec & start_p, hp::Vec & in_dir,
                                     cl_float3 & a, cl_float3 & b, cl_float3 c) {
    hp::Triangle tri(CL_FLOAT3_TO_VEC(a), CL_FLOAT3_TO_VEC(b), CL_FLOAT3_TO_VEC(c));
    return tri.intersect(start_p, in_dir);
}

inline float _box_intersect_dimension(float p0, float p, float s) {
    // if(p == 0) return 1/0;
    return (s - p0) / p;
}

#define SWAP_F(X, Y) \
    do { \
        float __tmp = (X); \
        (X) = (Y); \
        (Y) = __tmp; \
    } while(0) 

static bool _box_intersect(cl_float3 box_start, cl_float3 box_end, cl_float3 start_p, cl_float3 in_dir) {
    // start_p inside box
    if(start_p.s[0] >= box_start.s[0] && start_p.s[0] <= box_end.s[0] &&
       start_p.s[1] >= box_start.s[1] && start_p.s[1] <= box_end.s[1] &&
       start_p.s[2] >= box_start.s[2] && start_p.s[2] <= box_end.s[2]) return true;
    cl_float3 mins, maxs;

    mins.s[0] = _box_intersect_dimension(start_p.s[0], in_dir.s[0], box_start.s[0]);
    maxs.s[0] = _box_intersect_dimension(start_p.s[0], in_dir.s[0], box_end.s[0]);
    if(mins.s[0] > maxs.s[0]) SWAP_F(mins.s[0], maxs.s[0]);
    mins.s[1] = _box_intersect_dimension(start_p.s[1], in_dir.s[1], box_start.s[1]);
    maxs.s[1] = _box_intersect_dimension(start_p.s[1], in_dir.s[1], box_end.s[1]);
    if(mins.s[1] > maxs.s[1]) SWAP_F(mins.s[1], maxs.s[1]);
    mins.s[2] = _box_intersect_dimension(start_p.s[2], in_dir.s[2], box_start.s[2]);
    maxs.s[2] = _box_intersect_dimension(start_p.s[2], in_dir.s[2], box_end.s[2]);
    if(mins.s[2] > maxs.s[2]) SWAP_F(mins.s[2], maxs.s[2]);

    float max_of_mins = fmax(fmax(mins.s[0], mins.s[1]), mins.s[2]);
    float min_of_maxs = fmin(fmin(maxs.s[0], maxs.s[1]), maxs.s[2]);

    // printf("max of mins: %f, min of maxs: %f\n", max_of_mins, min_of_maxs);

    if(min_of_maxs <= 0) return false;

    return max_of_mins <= min_of_maxs;
}

#define ASSIGN_F3(X, Y) \
    do { \
        (X).s[0] = (Y)[0]; \
        (X).s[1] = (Y)[1]; \
        (X).s[2] = (Y)[2]; \
        (X).s[3] = 0; \
    } while(0)

TEST(SceneCLTest, box_intersect) {
    cl_float3 box_start, box_end;
    ASSIGN_F3(box_start, Vec(0, 0, 0));
    ASSIGN_F3(box_end, Vec(100, 100, 100));

    cl_float3 start_p, dir;
    ASSIGN_F3(start_p, Vec(50, 50, -100));
    ASSIGN_F3(dir, Vec(0, 0, 1));
    EXPECT_TRUE(_box_intersect(box_start, box_end, start_p, dir));
    ASSIGN_F3(dir, Vec(0, 0, -1));
    EXPECT_FALSE(_box_intersect(box_start, box_end, start_p, dir));

    ASSIGN_F3(start_p, Vec(-100, -100, -100));
    ASSIGN_F3(dir, (Vec(1, 1, 1).normalized()));
    EXPECT_TRUE(_box_intersect(box_start, box_end, start_p, dir));
    ASSIGN_F3(dir, (Vec(-1, 1, 1).normalized()));
    EXPECT_FALSE(_box_intersect(box_start, box_end, start_p, dir));
    ASSIGN_F3(dir, (Vec(-1, 0, 0).normalized()));
    EXPECT_FALSE(_box_intersect(box_start, box_end, start_p, dir));
    ASSIGN_F3(dir, (Vec(-1, -2, -3).normalized()));
    EXPECT_FALSE(_box_intersect(box_start, box_end, start_p, dir));

    ASSIGN_F3(start_p, Vec(-100, 50, -100));
    ASSIGN_F3(dir, (Vec(1, 0, 1).normalized()));
    EXPECT_TRUE(_box_intersect(box_start, box_end, start_p, dir));
}

std::pair<float, float> kdtree_intersect_test(hp::KDTree * scene, hp::Vec start_p, hp::Vec in_dir) {
    // auto start_p = hp::Vec(20, 34, -1000);
    // auto in_dir = hp::Vec(0, 0, 1);

    hp::Number result0 = -1;
    cl_int4 result0_geo;
    for(auto & geo_id: scene->geometries) {
        auto ret = triangle_intersect(start_p, in_dir,
                                      scene->points[geo_id.s[0]],
                                      scene->points[geo_id.s[1]],
                                      scene->points[geo_id.s[2]]);
        if(result0 == -1 || (ret >= 0 && ret < result0)) {
            result0 = ret;
            result0_geo = geo_id;
        }
    }

    hp::hp_log("Result0: geo: %d %d %d", result0_geo.s[0], result0_geo.s[1], result0_geo.s[2]);
    for(int i = 0 ; i < 3 ; i += 1) {
        auto point = scene->points[result0_geo.s[i]];
        hp::hp_log("   (%f %f %f)", point.s[0], point.s[1], point.s[2]);
    }

    cl_float3 start_p_cl;
    ASSIGN_F3(start_p_cl, start_p);
    cl_float3 in_dir_cl;
    ASSIGN_F3(in_dir_cl, in_dir);

    auto kdtree_data = scene->getData();
    auto v_kd_node_header = kdtree_data.first;
    auto v_kd_leaf_data = kdtree_data.second;

    std::vector<int> match_datas;
    int node_index = 0;
    int come_from_child = 0;
    while(node_index < v_kd_node_header.size()) {
        hp::hp_log("Visiting node %d, come_from_child = %d", node_index, come_from_child);
        int goto_child = 0;
        auto node = v_kd_node_header[node_index];
        if(come_from_child == 0) {
            if(_box_intersect(node.box_start, node.box_end, start_p_cl, in_dir_cl)) {
                hp::hp_log("Box intersect!");
                if(node.child < 0) {
                    if(node.data >= 0)
                        match_datas.push_back(node.data);
                }
                else
                    goto_child = 1;
            }
        }

        if(goto_child) {
            come_from_child = 0;
            // hp::hp_log("Going to child...");
            node_index = node.child;
        } else {
            if(node.sibling >= 0) {
                // hp::hp_log("I have sibling, go there");
                come_from_child = 0;
                node_index = node.sibling;
            } else if(node.parent >= 0) {
                // hp::hp_log("I have parent, go there");
                come_from_child = 1;
                node_index = node.parent;
            } else
                break;
        }
    }

    // hp::hp_log("All node visited");

    hp::Number result1 = -1;
    for(auto & data_id: match_datas) {
        cl_int * data = v_kd_leaf_data.data() + data_id;
        int data_size = data[0];
        // hp::hp_log("data_size: %d", data_size);
        for(int x = 0 ; x < data_size ; x += 1) {
            cl_int triangle_id = data[x+1];
            auto triangle = scene->geometries[triangle_id];
            hp::hp_log("Checking triangle: %d %d %d", triangle.s[0], triangle.s[1], triangle.s[2]);
            auto ret = triangle_intersect(start_p, in_dir, 
                                          scene->points[triangle.s[0]],
                                          scene->points[triangle.s[1]],
                                          scene->points[triangle.s[2]]);
            if(result1 == -1 || (ret >= 0 && ret < result1))
                result1 = ret;
        }
    }

    hp::hp_log("result0 vs result1: %f %f", result0, result1);

    return std::make_pair(result0, result1);
}

TEST(SceneCLTest, kdtree_single) {
    auto scene = std::make_unique<hp::KDTree>(std::string("teapot.obj"));
    hp::Vec view_p(-35.000000, -20.000000, -500.000000);
    hp::Vec dir(0, 0, 1);
    auto ret = kdtree_intersect_test(scene.get(), view_p, dir);
    EXPECT_TRUE(std::abs(ret.first - ret.second) < 0.1);
}

TEST(SceneCLTest, kdtree_intersect) {
    auto scene = std::make_unique<hp::KDTree>(std::string("teapot.obj"));
    for(int i = 0 ; i < 100 ; i += 1) {
        for(int j = 0 ; j < 100 ; j += 1) {
            hp::Vec view_p(i-50, j-50, -500);
            Vec dir = Vec(0, 0, 1);
            // dir.normalize();
            hp::hp_log("(%f %f %f)", view_p[0], view_p[1], view_p[2]);
            auto ret = kdtree_intersect_test(scene.get(), view_p, dir);
            EXPECT_TRUE(std::abs(ret.first - ret.second) < 0.1);
        }
    }
}
