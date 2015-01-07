/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-07
*/

#include <iostream>
#include <fstream>
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
        mat1.ambient = {10, 10, 10};
        scene->setMaterial(1, mat1);
    }
    scene->setGeometry(0, std::make_unique<Sphere>(Vec(200, 200, 300), 100), 0);
    scene->setGeometry(1, std::make_unique<Sphere>(Vec(500, 200, 200), 100), 1);

    Vec view_p(200, 200, -500);
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

    std::ofstream fout("out.ppm");
    fout << "P3\n";
    fout << "512 512\n255\n";
    for(int i = 0 ; i < 512 ; i += 1) {
        for(int j = 0 ; j < 512 ; j += 1) {
            auto vec = runner->result[i + j * 512];
            for(int k = 0 ; k < 3 ; k += 1)
                fout << int(255 * (1.0 - std::exp(-vec[k]))) << " ";
        }
        fout << "\n";
    }
    fout.close();
}
