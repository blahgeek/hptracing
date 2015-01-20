/* 
* @Author: BlahGeek
* @Date:   2015-01-18
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-20
*/

#include <iostream>
#include <fstream>
#include <string>
#include "hp/common.h"
#include "hp/trace_runner.h"
#include "hp/scene/kdtree.h"

#include "OptionParser/OptionParser.h"

using namespace std;
using namespace hp;
using optparse::OptionParser;

int main(int argc, char const *argv[]) {
    OptionParser parser = OptionParser().description("HPTracing 0.0.1");
    parser.add_option("-i", "--input").dest("input");
    parser.add_option("--width").dest("width").type("int").set_default(500);
    parser.add_option("--height").dest("height").type("int").set_default(500);
    parser.add_option("--view").dest("view").set_default("0,0,-500");
    parser.add_option("--rotate-x").dest("rotate-x").type("float").set_default(0.0);
    parser.add_option("--rotate-y").dest("rotate-y").type("float").set_default(0.0);
    parser.add_option("-x", "--supersample").dest("supersample").action("store_true").set_default("0");
    parser.add_option("-s", "--sample").dest("sample").type("int").set_default(10);
    parser.add_option("-d", "--depth").dest("depth").type("int").set_default(6);
    parser.add_option("--brightness").dest("brightness").type("float").set_default(1.0);
    parser.add_option("--no-diffuse").dest("no-diffuse").action("store_true").set_default("0");
    parser.add_option("-o", "--output").dest("output").set_default("out.ppm");

    optparse::Values options = parser.parse_args(argc, argv);
    vector<string> args = parser.args();

    TickTock timer;

    auto scene = std::make_unique<KDTree>(options["input"]);
    timer.timeit("Build KDTree done.");
    
    string view_point_str = options["view"];
    cl_float3 view_point;
    for(int i = 0 ; i < 3 ; i += 1) {
        view_point.s[i] = atof(view_point_str.c_str());
        if(i < 2)
            view_point_str = view_point_str.substr(view_point_str.find(',')+1);
    }
    hp_log("Viewing at (%f %f %f)", view_point.s[0], view_point.s[1], view_point.s[2]);

    Eigen::Matrix<float, 3, 3> rotate_x;
    float angle_x = (float)options.get("rotate-x") / 180.0 * PI;
    rotate_x << 1, 0, 0, 
                0, std::cos(angle_x), -std::sin(angle_x),
                0, std::sin(angle_x), std::cos(angle_x);
    Eigen::Matrix<float, 3, 3> rotate_y;
    float angle_y = (float)options.get("rotate-y") / 180.0 * PI;
    rotate_y << std::cos(angle_y), 0, std::sin(angle_y),
                0, 1, 0,
                -std::sin(angle_y), 0, std::cos(angle_y);
    Eigen::Matrix<float, 3, 3> rotate = rotate_x * rotate_y;

    std::vector<cl_float3> view_dirs;

    bool supersample = options.get("supersample");

    int width = (int)options.get("width");
    int height = (int)options.get("height");
    for(int i = 0 ; i < width ; i += 1) {
        for(int j = 0 ; j < height ; j += 1) {
            for(int ii = 0 ; ii < (supersample?2:1) ; ii += 1) {
                for(int jj = 0 ; jj < (supersample?2:1) ; jj += 1) {
                    Vec dir = Vec(i - width / 2 + ii * 0.5, 
                                  j - height / 2 + jj * 0.5, 
                                  width * 2);
                    dir.normalize();
                    dir = rotate * dir;
                    cl_float3 x;
                    ASSIGN_F3(x, dir);
                    view_dirs.push_back(x);
                }
            }
        }
    }
    timer.timeit("Prepare view rays done");

    auto runner = std::make_unique<TraceRunner>(scene);
    timer.timeit("Init OpenCL hardware & memories done");

    auto result = runner->run(view_dirs, view_point, 
                              (int)options.get("sample"), 
                              (int)options.get("depth"),
                              (int)options.get("no-diffuse"));

    timer.timeit("1 image generated");

    auto output_filename = options["output"];
    std::ofstream fout(output_filename.c_str());
    fout << "P3\n";
    fout << width << " " << height << "\n255\n";
    for(int y = height -1 ; y >= 0 ; y -= 1) {
        for(int x = width - 1 ; x >= 0 ; x -= 1) {
            for(int k = 0 ; k < 3 ; k += 1) {
                float val = 0;
                if(!supersample)
                    val = result[(x * height + y) * 3 + k];
                else {
                    for(int i = 0 ; i < 4 ; i += 1)
                        val += result[((x * height + y) * 4 + i) * 3 + k];
                    val /= 4.0;
                }
                val *= (float)options.get("brightness");
                fout << int(255 * (1.0 - std::exp(-val))) << " ";
            }
        }
        fout << "\n";
    }
    fout.close();

    timer.timeit("Write to output done");

    return 0;
}

