/* 
* @Author: BlahGeek
* @Date:   2015-01-18
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-21
*/

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

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

// runner
std::unique_ptr<TraceRunner> runner = nullptr;
// options
cl_float3 view_point;
cl_float3 up_dir, right_dir;
int width, height;
float angle;
bool supersample;
int sample, depth;
bool no_diffuse;
float brightness;
// results
unsigned char * pixels = nullptr;
int pixels_size = 0;

bool need_rerun = true;

static void runit() {
    if(!runner) {
        hp_log("WARNING: Runner is not ready");
        return;
    }
    if(pixels == nullptr || pixels_size != width * height * 4) {
        if(pixels) delete [] pixels;
        pixels_size = width * height * 4;
        pixels = new unsigned char [pixels_size];
    }
    hp_log("Rendering image... %dx%d, %d samples, max-depth %d",
           width, height, sample, depth);
    TickTock timer;
    runner->run(pixels, view_point, up_dir, right_dir,
                float(width) / float(height) * angle, angle,
                width, height, supersample?2:1, supersample?2:1,
                sample, depth, no_diffuse?1:0, brightness);
    timer.timeit("Render done");
}

static void displayFunc() {
    if(pixels == nullptr) return;
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos2i(0, 0);
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glutSwapBuffers();
}

static void idleFunc() {
    if(!need_rerun) return;
    runit();
    need_rerun = false;
    glutPostRedisplay();
}

int main(int argc, char **argv) {
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
    runner = std::make_unique<TraceRunner>(scene);
    timer.timeit("Init OpenCL hardware & memories done");

    view_point = str2float3(options["view"]);
    up_dir = str2float3(options["up"]);
    right_dir = str2float3(options["right"]);
    width = (int)options.get("width");
    height = (int)options.get("height");
    angle = (float)options.get("angle");
    supersample = options.get("supersample");
    sample = (int)options.get("sample");
    depth = (int)options.get("depth");
    no_diffuse = (bool)options.get("no-diffuse");
    brightness = (float)options.get("brightness");

    // init GLUT
    glutInitWindowSize(width, height);
    glutInitWindowPosition(0,0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInit(&argc, argv);
    glutCreateWindow("hpTracing by BlahGeek");
    glutDisplayFunc(displayFunc);
    glutIdleFunc(idleFunc);
    glViewport(0, 0, width, height);
    glLoadIdentity();
    glOrtho(0.f, width - 1.f, 0.f, height - 1.f, -1.f, 1.f);
    
    // runit();

    glutMainLoop();

    // auto output_filename = options["output"];
    // std::ofstream fout(output_filename.c_str());
    // fout << "P3\n";
    // fout << width << " " << height << "\n255\n";
    // for(int i = 0 ; i < height ; i += 1) {
    //     for(int j = 0 ; j < width ; j += 1) {
    //         for(int k = 0 ; k < 3 ; k += 1)
    //             fout << int(pixels[(i * width + j) * 4 + k]) << " ";
    //     }
    //     fout << "\n";
    // }
    // fout.close();

    // timer.timeit("Write to output done");

    return 0;
}

