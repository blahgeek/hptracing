/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-06
*/

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include "./hp/geometry/sphere.h"

using namespace std;
using namespace hp;

TEST(GeometryTest, sphere) {
    auto sphere = std::make_unique<Sphere>(Vec(0, 0, 0), 1);
    auto normal = sphere->getNormal(Vec(0, 1, 0));
    EXPECT_EQ(normal, Vec(0, 1, 0));

    auto p = Vec(2, 3, 4); p.normalize();
    auto ret = sphere->intersect(Vec(0, 0, 0), p);
    EXPECT_TRUE(std::abs(ret - 1) < 1e-3);

    ret = sphere->intersect(p * 0.2, p);
    EXPECT_TRUE(std::abs(ret - 0.8) < 1e-3);

    ret = sphere->intersect(p * (-1.2), p);
    EXPECT_TRUE(std::abs(ret - 0.2) < 1e-3);

    ret = sphere->intersect(p * 10, p);
    EXPECT_TRUE(ret < 0);

    ret = sphere->intersect(Vec(-100, 100, 100), p);
    EXPECT_TRUE(ret < 0);

    ret = sphere->intersect(p, p);
    EXPECT_TRUE(ret < 0);

    ret = sphere->intersect(p, -p);
    EXPECT_TRUE(std::abs(ret - 2) < 1e-3);
}
