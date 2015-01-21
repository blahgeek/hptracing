/* 
* @Author: BlahGeek
* @Date:   2015-01-18
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-21
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

static cl_float3 str2float3(std::string s) {
    cl_float3 ret;
    for(int i = 0 ; i < 3 ; i += 1) {
        ret.s[i] = atof(s.c_str());
        if(i < 2)
            s = s.substr(s.find(',')+1);
    }
    return ret;
}

int main(int argc, char const *argv[]) {
    OptionParser parser = OptionParser().description("HPTracing 0.0.1");
    parser.add_option("-i", "--input").dest("input");
    parser.add_option("--width").dest("width").type("int").set_default(500);
    parser.add_option("--height").dest("height").type("int").set_default(500);
    parser.add_option("--view").dest("view").set_default("0,0,-500");
    parser.add_option("--up").dest("up").set_default("0,1,0");
    parser.add_option("--right").dest("right").set_default("1,0,0");
    parser.add_option("--angle").dest("angle").type("float").set_default("1.0");
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
    
    bool supersample = options.get("supersample");
    int width = (int)options.get("width");
    int height = (int)options.get("height");

    auto runner = std::make_unique<TraceRunner>(scene);
    timer.timeit("Init OpenCL hardware & memories done");

    auto result = runner->run(str2float3(options["view"]), 
                              str2float3(options["up"]), 
                              str2float3(options["right"]),
                              float(width) / float(height) * (float)options.get("angle"),
                              (float)options.get("angle"),
                              width, height,
                              supersample ? 2 : 1,
                              supersample ? 2 : 1,
                              (int)options.get("sample"),
                              (int)options.get("depth"),
                              (int)options.get("no-diffuse"),
                              (float)options.get("brightness"));

    timer.timeit("1 image generated");

    auto output_filename = options["output"];
    std::ofstream fout(output_filename.c_str());
    fout << "P3\n";
    fout << width << " " << height << "\n255\n";
    for(int i = 0 ; i < height ; i += 1) {
        for(int j = 0 ; j < width ; j += 1) {
            for(int k = 0 ; k < 3 ; k += 1)
                fout << int(result[(i * width + j) * 4 + k]) << " ";
        }
        fout << "\n";
    }
    fout.close();

    timer.timeit("Write to output done");

    return 0;
}

