/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-10
*/

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include "./hp/geometry/sphere.h"
#include "./hp/geometry/triangle.h"
#include "./hp/scene/base.h"

using namespace std;
using namespace hp;

TEST(SceneTest, base) {
    auto scene = std::make_unique<Scene>(2, 2);
    {
        Material mat0;
        mat0.diffuse = {1, 0, 0};
        mat0.specular = {1, 1, 1};
        scene->addMaterial(mat0);
    }
    {
        Material mat1;
        mat1.ambient = {10, 10, 10};
        scene->addMaterial(mat1);
    }
    scene->addGeometry(std::make_unique<Sphere>(Vec(200, 200, 200), 100), 0);
    scene->addGeometry(std::make_unique<Sphere>(Vec(500, 200, 300), 100), 1);

    Vec p(-0.123051, -0.00992351, 0.992351); p.normalize();
    auto result = scene->intersect(Vec(200, 200, -500), p);

    auto ret = std::get<0>(result);
    cerr << "Ret: " << ret << endl;

    EXPECT_TRUE(ret > 0);
    Vec intersect_p = Vec(200, 200, -500) + ret * p;
    cerr << "p[2]: " << intersect_p[2] << endl;
    EXPECT_TRUE(intersect_p[2] < 200);

}

TEST(SceneTest, load) {
    auto scene = std::make_unique<Scene>(std::string("obj/cornell_box.obj"));
    Vec p(-0.123051, -0.00992351, 0.992351); p.normalize();
    auto result = scene->intersect(Vec(200, 200, -500), p);
    auto ret = std::get<0>(result);
    cerr << "Ret: " << ret << endl;
}

