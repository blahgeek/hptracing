/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-13
*/

#include <iostream>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <algorithm>
#include "./hp/scene_cl/base.h"
#include "./hp/scene_cl/uniform_box.h"

using namespace std;

TEST(SceneCLTest, load) {
    auto scene = std::make_unique<hp::cl::Scene>(std::string("obj/cornell_box.obj"));
    EXPECT_EQ(scene->lights.size(), 256);
    auto half_count = std::count_if(scene->lights.begin(), scene->lights.end(),
                                   [](const cl_int4 & x)->bool {
                                      return x.s[1] == 14;
                                   });
    EXPECT_EQ(half_count, 127);
}

TEST(SceneCLTest, uniform_box_load) {
    auto scene = std::make_unique<hp::cl::UniformBox>(std::string("obj/cornell_box.obj"));
    EXPECT_TRUE(bool(scene));
}
