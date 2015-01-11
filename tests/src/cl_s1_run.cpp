/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-11
*/

#include <iostream>
#include <gtest/gtest.h>
#include <fstream>

#include "./hp/common.h"
#include "./hp/cl_program.h"
#include "./hp/scene_cl/base.h"
#include <sys/time.h>

using namespace std;
using namespace hp;

TEST(CLTest, s1_run) {
    auto cl_program = std::make_unique<hp::CLProgram>();

    auto kernel = cl_program->getKernel("s1_run");
    auto scene = std::make_unique<hp::cl::Scene>(std::string("obj/cornell_box.obj"));

    cl_mem v_sizes_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                  sizeof(cl_int) * 10, nullptr);
    cl_mem s1_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                             sizeof(cl::unit_S1) * 100000, nullptr);
    cl_mem s2_refract_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                     sizeof(cl::unit_S2_refract) * 100000, nullptr);
    cl_mem s2_specular_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                     sizeof(cl::unit_S2_specular) * 100000, nullptr);
    cl_mem s2_diffuse_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                     sizeof(cl::unit_S2_diffuse) * 100000, nullptr);
    cl_mem s2_light_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                   sizeof(cl::unit_S2_light) * 100000, nullptr);
    // Result data
    cl_mem results_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                  sizeof(cl_float) * 6, nullptr);
    cl_mem points_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                 sizeof(cl_float3) * scene->points.size(),
                                                 scene->points.data());
    cl_mem materials_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                    sizeof(cl::Material) * scene->materials.size(),
                                                    scene->materials.data());

#define ASSIGN_F3(X, Y) \
    do { \
        (X).s[0] = (Y)[0]; \
        (X).s[1] = (Y)[1]; \
        (X).s[2] = (Y)[2]; \
    } while(0)

    cl::unit_S1 unit;
    unit.orig_id = 0;
    unit.depth = 0;
    unit.strength.s[0] = unit.strength.s[1] = unit.strength.s[2] = 1;
    unit.geometry.s[0] = 20;
    unit.geometry.s[1] = 21;
    unit.geometry.s[2] = 22;
    unit.geometry.s[3] = 5; // light
    ASSIGN_F3(unit.start_p, Vec(200, 200, -500));
    ASSIGN_F3(unit.in_dir, Vec(-0.123051, -0.00992351, 0.992351));
    unit.intersect_number = 1067.36;

    cl::unit_S1 units[2]; units[0] = unit; units[1] = unit;
    // units[1].orig_id = 1;

    cl_program->writeBuffer(s1_mem, sizeof(unit) * 2, units);
    cl_int v_sizes[10] = {0};
    v_sizes[1] = 2;
    cl_program->writeBuffer(v_sizes_mem, sizeof(v_sizes), v_sizes);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &v_sizes_mem);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &s1_mem);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &results_mem);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &s2_refract_mem);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &s2_specular_mem);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &s2_diffuse_mem);
    clSetKernelArg(kernel, 6, sizeof(cl_mem), &s2_light_mem);
    clSetKernelArg(kernel, 7, sizeof(cl_mem), &points_mem);
    clSetKernelArg(kernel, 8, sizeof(cl_mem), &materials_mem);

    size_t global = 100;
    size_t local = 10;
    clEnqueueNDRangeKernel(cl_program->commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
    clFinish(cl_program->commands);

    cl_program->readBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);

    cl::unit_S2_diffuse s2_diffuse[2];
    cl_program->readBuffer(s2_diffuse_mem, sizeof(cl::unit_S2_diffuse) * 2, s2_diffuse);

    cl_float result[6];
    cl_program->readBuffer(results_mem, sizeof(cl_float) * 6, result);

    __builtin_trap();

}
