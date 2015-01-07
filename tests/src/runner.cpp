/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-07
*/

#include <iostream>
#include <gtest/gtest.h>

#include "hp/trace_runner.h"
#include "hp/scene/base.h"

#include "hp/geometry/sphere.h"

using namespace hp;

TEST(Runner, run) {
    auto scene = std::make_unique<Scene>(2, 2);
    {
        Material mat0;
        mat0.diffuse = {1, 0, 0};
        mat0.specular = {1, 1, 1};
        scene->setMaterial(0, mat0);
    }
    {
        Material mat1;
        mat1.ambient = {1, 1, 1};
        scene->setMaterial(1, mat1);
    }
    scene->setGeometry(0, std::make_unique<Sphere>(Vec(200, 200, 200), 100), 0);
    scene->setGeometry(1, std::make_unique<Sphere>(Vec(500, 200, 300), 100), 1);

    Vec view_p(400, 200, -500);
    std::vector<Vec> view_dir;
    for(int i = 0 ; i < 512 ; i += 1) {
        for(int j = 0 ; j < 512 ; j += 1) {
            Vec dir = Vec(i, j, 0) - view_p;
            view_dir.push_back(dir.normalized());
        }
    }

    auto runner = std::make_unique<TraceRunner>(std::move(scene),
                                                std::move(view_dir),
                                                view_p);
    runner->run();
}
