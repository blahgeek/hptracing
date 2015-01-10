/* 
* @Author: BlahGeek
* @Date:   2015-01-06
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-10
*/

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include "./hp/geometry/sphere.h"
#include "./hp/geometry/triangle.h"

using namespace std;
using namespace hp;

#define EXPECT_VEC_EQ(x, y) \
    EXPECT_TRUE(std::abs(((x)-(y)).norm()) < 1e-3)

TEST(GeometryTest, sphere) {
    auto sphere = std::make_unique<Sphere>(Vec(0, 0, 0), 1);
    auto normal = sphere->getNormal(Vec(0, 1, 0));
    EXPECT_VEC_EQ(normal, Vec(0, 1, 0));

    auto p = Vec(2, 3, 4); p.normalize();
    auto ret = sphere->intersect(Vec(0, 0, 0), p);
    EXPECT_TRUE(std::abs(ret - 1) < 1e-3);

    ret = sphere->intersect(p * 0.2, p);
    EXPECT_TRUE(std::abs(ret - 0.8) < 1e-3);

    ret = sphere->intersect(p * (-1.7), p);
    EXPECT_TRUE(std::abs(ret - 0.7) < 1e-3);

    ret = sphere->intersect(p * 10, p);
    EXPECT_TRUE(ret < 0);

    ret = sphere->intersect(Vec(-100, 100, 100), p);
    EXPECT_TRUE(ret < 0);

    ret = sphere->intersect(p, p);
    EXPECT_TRUE(ret < 0);

    ret = sphere->intersect(p, -p);
    EXPECT_TRUE(std::abs(ret - 2) < 1e-3);

    sphere = std::make_unique<Sphere>(Vec(200, 200, 200), 100);
    p = Vec(-0.123051, -0.00992351, 0.992351); p.normalize();
    ret = sphere->intersect(Vec(200, 200, -500), p);
    EXPECT_TRUE(ret > 0);
    auto intersect_p = Vec(200, 200, -500) + ret * p;
    EXPECT_TRUE(intersect_p[2] < 200);
}

TEST(GeometryTest, triangle) {
    auto triangle = std::make_unique<Triangle>(Vec(0, 0, 0),
                                               Vec(1, 0, 0),
                                               Vec(0, 1, 0));
    auto normal = triangle->getNormal(Vec(0, 0, 0));
    EXPECT_VEC_EQ(normal, Vec(0, 0, 1));

    auto ret = triangle->intersect(Vec(1/3, 2/3, 2/7), Vec(0.6, 0.8, 0));
    EXPECT_TRUE(ret < 0);

    auto p = Vec(2, 3, 4); p.normalize();
    ret = triangle->intersect(Vec(0, 0, 0), p);
    EXPECT_TRUE(ret < 0);

    ret = triangle->intersect(Vec(0.3, 0.3, 0)-p, p);
    EXPECT_TRUE(std::abs(ret - 1) < 1e-3);

    ret = triangle->intersect(Vec(0.5, 0.5, 0)+p, p);
    EXPECT_TRUE(ret < 0);

    ret = triangle->intersect(Vec(-3, -3, 0), p);
    EXPECT_TRUE(ret < 0);

    triangle = std::make_unique<Triangle>(Vec(200, 500, 0),
                                          Vec(250, 400, 0),
                                          Vec(150, 400, 0));
    p = Vec(-0.092450, 0.369800, 0.924500); p.normalize();
    Vec start(250, 250, -500);
    ret = triangle->intersect(start, p);
    EXPECT_TRUE(ret > 0);
}

TEST(Geometry, triangle_speed_test) {
    auto triangle = std::make_unique<Triangle>(Vec(0,0,0), 
                                               Vec(1000, 0, 0),
                                               Vec(0, 1000, 0));
    Vec start_p(500, 500, -500);
    int ret = 0;
    for(int i = 0 ; i < 1000 ; i += 1) {
        for(int j = 0 ; j < 1000 ; j += 1) {
            Vec dir(i-500, j-500, 500); dir.normalize();
            Number result = triangle->intersect(start_p, dir);
            if(result > 0) ret += 1;
        }
    }
    EXPECT_TRUE(std::abs(ret - 500000) < 1000);
}

TEST(GeometryTest, reflection) {
    auto ret = Geometry::getReflection(Vec(0.6, 0.8, 0),
                                       Vec(0, 1, 0));
    EXPECT_VEC_EQ(ret, Vec(0.6, -0.8, 0));

    ret = Geometry::getReflection(Vec(0, 1, 0),
                                  Vec(0, 1, 0));
    EXPECT_VEC_EQ(ret, Vec(0, -1, 0));

    ret = Geometry::getReflection(Vec(1, 0, 0),
                                  Vec(0, 1, 0));
    EXPECT_VEC_EQ(ret, Vec(1, 0, 0));
}

TEST(GeometryTest, refraction) {
    auto ret = Geometry::getRefraction(Vec(0.6, 0.8, 0),
                                       Vec(0, 1, 0), 1);
    // cerr << ret[0] << " " << ret[1] << " " << ret[2] << endl;
    EXPECT_VEC_EQ(ret, Vec(0.6, 0.8, 0));

    ret = Geometry::getRefraction(Vec(0.6, -0.8, 0),
                                  Vec(0, 1, 0), 1);
    EXPECT_VEC_EQ(ret, Vec(0.6, -0.8, 0));

    ret = Geometry::getRefraction(Vec(std::sqrt(0.5), -std::sqrt(0.5), 0),
                                  Vec(0, 1, 0), std::sqrt(0.5));
    EXPECT_VEC_EQ(ret, Vec(0.5, -0.5*std::sqrt(3), 0));

    ret = Geometry::getRefraction(Vec(0, -1, 0),
                                  Vec(0, 1, 0), std::sqrt(0.5));
    EXPECT_VEC_EQ(ret, Vec(0, -1, 0));

    ret = Geometry::getRefraction(Vec(0.5, 0.5*std::sqrt(3), 0),
                                  Vec(0, 1, 0), std::sqrt(0.5));
    EXPECT_VEC_EQ(ret, Vec(std::sqrt(0.5), std::sqrt(0.5), 0));

    ret = Geometry::getRefraction(Vec(1, 0, 0),
                                  Vec(0, 1, 0), std::sqrt(0.5));
    EXPECT_VEC_EQ(ret, Vec(std::sqrt(0.5), -std::sqrt(0.5), 0));

    // no refraction
    ret = Geometry::getRefraction(Vec(0.8, std::sqrt(1-0.8*0.8), 0),
                                  Vec(0, 1, 0), std::sqrt(0.5));
    EXPECT_TRUE(std::isnan(ret[0]));
}
