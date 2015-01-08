/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-08
*/

#include <iostream>
#include <fstream>
#include <gtest/gtest.h>

#include "hp/trace_runner.h"
#include "hp/scene/base.h"

#include "hp/geometry/sphere.h"
#include "hp/geometry/triangle.h"

using namespace hp;

TEST(Runner, run) {
    // auto scene = std::make_unique<Scene>(5, 3);
    // {
    //     Material mat0;
    //     mat0.diffuse = {1, 0, 0};
    //     mat0.specular = {0.1, 0.1, 0.1};
    //     mat0.specular_exp = 2;
    //     scene->setMaterial(0, mat0);
    // }
    // {
    //     Material mat0;
    //     mat0.diffuse = {0.2, 0.2, 0.2};
    //     mat0.specular = {0.5, 0.5, 0.5};
    //     mat0.specular_exp = 10;
    //     mat0.dissolve = 0;
    //     scene->setMaterial(2, mat0);
    // }
    // {
    //     Material mat1;
    //     mat1.ambient = {40, 40, 40};
    //     scene->setMaterial(1, mat1);
    // }
    // scene->setGeometry(0, std::make_unique<Sphere>(Vec(200, 200, 400), 100), 0);
    // scene->setGeometry(2, std::make_unique<Sphere>(Vec(500, 200, 400), 100), 0);
    // scene->setGeometry(3, std::make_unique<Sphere>(Vec(200, 200, 100), 100), 2);
    // scene->setGeometry(1, std::make_unique<Sphere>(Vec(1000, 100, -1000), 400), 1);
    // scene->setGeometry(4, std::make_unique<Triangle>(Vec(200, 500, 0),
    //                                                  Vec(250, 400, 0),
    //                                                  Vec(150, 400, 0)), 0);
    auto scene = std::make_unique<Scene>(std::string("obj/cornell_box.obj"));

    Vec view_p(250, 250, -500);
    std::vector<Vec> view_dir;
    for(int i = 0 ; i < 500 ; i += 1) {
        for(int j = 0 ; j < 500 ; j += 1) {
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
    fout << "500 500\n255\n";
    for(int i = 0 ; i < 500 ; i += 1) {
        for(int j = 0 ; j < 500 ; j += 1) {
            auto vec = runner->result[i + j * 500];
            for(int k = 0 ; k < 3 ; k += 1)
                fout << int(255 * (1.0 - std::exp(-vec[k]))) << " ";
        }
        fout << "\n";
    }
    fout.close();
}
