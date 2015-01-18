/* 
* @Author: BlahGeek
* @Date:   2015-01-07
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-18
*/

#include <iostream>
#include <fstream>
#include <gtest/gtest.h>

#include "hp/trace_runner.h"
#include "hp/scene/kdtree.h"

using namespace hp;

#define ASSIGN_F3(X, Y) \
    do { \
        (X).s[0] = (Y)[0]; \
        (X).s[1] = (Y)[1]; \
        (X).s[2] = (Y)[2]; \
        (X).s[3] = 0; \
    } while(0)

TEST(CLRunner, run) {
    auto scene = std::make_unique<KDTree>(std::string("teapot.obj"));

    Vec _view_p(0, 0, -500);
    cl_float3 view_p;
    ASSIGN_F3(view_p, _view_p);
    std::vector<cl_float3> view_dir;
    // std::vector<Vec> view_dir;
    for(int i = 0 ; i < 1000 ; i += 1) {
        for(int j = 0 ; j < 1000 ; j += 1) {
            Vec dir = Vec((float(i)/2-250)/2, (float(j)/2-250)/2, 0) - _view_p;
            dir.normalize();
            cl_float3 x;
            ASSIGN_F3(x, dir);
            view_dir.push_back(x);
        }
    }

    auto runner = std::make_unique<TraceRunner>(std::move(scene),
                                                std::move(view_dir),
                                                view_p);
    runner->run();

    std::ofstream fout("out.ppm");
    fout << "P3\n";
    fout << "500 500\n255\n";
    for(int y = 500 - 1 ; y >= 0 ; y --) {
        for(int x = 0 ; x < 500 ; x ++) {
            for(int k = 0 ; k < 3 ; k += 1) {
                auto val = runner->result[((x*2) * 1000 + (y*2)) * 3 + k];
                val += runner->result[((x*2+1) * 1000 + (y*2)) * 3 + k];
                val += runner->result[((x*2) * 1000 + (y*2+1)) * 3 + k];
                val += runner->result[((x*2+1) * 1000 + (y*2+1)) * 3 + k];
                val /= 4;
                fout << int(255 * (1.0 - std::exp(-val))) << " ";
            }
        }
        fout << std::endl;
    }
    fout.close();
}
