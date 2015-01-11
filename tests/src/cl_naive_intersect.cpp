/* 
* @Author: BlahGeek
* @Date:   2015-01-09
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-11
*/

#include <iostream>
#include <gtest/gtest.h>
#include <fstream>

#include "./hp/common.h"
#include "./hp/cl_program.h"
#include <sys/time.h>

uint64_t GetTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

using namespace hp;

TEST(CLTest, naive_intersect) {
    hp::CLProgram cl_program;

    auto kernel = cl_program.getKernel("naive_intersect");

    cl_float scene_points[] = {0.0, 0.0, 0.0, 0.0,
                               1000.0, 0.0, 0.0, 0.0,
                               0.0, 1000.0, 0.0, 0.0};
    cl_int scene_mesh[] = {0, 1, 2, 0};

    cl_int scene_mesh_size = 1;

    cl::unit_S0 * v_s0 = new cl::unit_S0[1000000];
    // cl_int v_s0_size = 1000000;
    cl::unit_S1 * v_s1 = new cl::unit_S1[1000000];
    // unit_S1 v_s1[1000000];
    // cl_int v_s1_size = 0;

    for(int i = 0 ; i < 1000 ; i += 1) {
        for(int j = 0 ; j < 1000 ; j += 1) {
            hp::Vec p(i - 500, j - 500, 500); p.normalize();
            int index = i * 1000 + j;
            v_s0[index].orig_id = index;
            v_s0[index].depth = 0;
            v_s0[index].strength.s[0] = 1;
            v_s0[index].strength.s[1] = 1;
            v_s0[index].strength.s[2] = 1;
            v_s0[index].start_p.s[0] = 500;
            v_s0[index].start_p.s[1] = 500;
            v_s0[index].start_p.s[2] = -500;
            v_s0[index].in_dir.s[0] = p[0];
            v_s0[index].in_dir.s[1] = p[1];
            v_s0[index].in_dir.s[2] = p[2];
        }
    }

    cl_int v_sizes[10] = {1000000, 0, 0};

    cl_mem v_sizes_mem = cl_program.createBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                                sizeof(v_sizes), v_sizes);
    cl_mem s0_mem = cl_program.createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                            sizeof(cl::unit_S0) * 1000000, v_s0);
    cl_mem s1_mem = cl_program.createBuffer(CL_MEM_WRITE_ONLY , 
                                            sizeof(cl::unit_S1) * 1000000, NULL);
    cl_mem scene_points_mem = cl_program.createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                                      sizeof(scene_points), scene_points);
    cl_mem scene_mesh_mem = cl_program.createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                                    sizeof(scene_mesh), scene_mesh);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &v_sizes_mem);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &s0_mem);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &s1_mem);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &scene_points_mem);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &scene_mesh_mem);
    clSetKernelArg(kernel, 5, sizeof(cl_int), &scene_mesh_size);

    auto t0 = GetTimeStamp();

    size_t global = 1000200;
    size_t local = 200;
    clEnqueueNDRangeKernel(cl_program.commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
    clFinish(cl_program.commands);

    clReleaseKernel(kernel);

    clEnqueueReadBuffer(cl_program.commands, v_sizes_mem, CL_TRUE, 0, sizeof(cl_int) * 10, v_sizes, 0, NULL, NULL);
    auto t1 = GetTimeStamp();
    clEnqueueReadBuffer(cl_program.commands, s1_mem, CL_TRUE, 0, sizeof(cl::unit_S1) * 100000, v_s1, 0, NULL, NULL);

    std::cout << "TIME: " << t1 - t0 << std::endl;
    std::cout << "SIZE: " << v_sizes[1] << std::endl;

    EXPECT_TRUE(std::abs(v_sizes[1] - 500000) < 2000);
    EXPECT_TRUE(v_s1[0].strength.s[0] == 1);
    __builtin_trap();

}

#include "./hp/scene_cl/base.h"

TEST(CLTest, intersect2) {
    auto cl_program = std::make_unique<hp::CLProgram>();

    auto scene = std::make_unique<cl::Scene>(std::string("obj/cornell_box.obj"));

    cl_mem points_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                 sizeof(cl_float3) * scene->points.size(),
                                                 scene->points.data());
    cl_int geometries_size = scene->geometries.size();
    cl_mem geometries_mem = cl_program->createBuffer(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                                     sizeof(cl_int4) * scene->geometries.size(),
                                                     scene->geometries.data());
    // Units...
    cl_mem v_sizes_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                                  sizeof(cl_int) * 10, nullptr);
    cl_mem s0_mem = cl_program->createBuffer(CL_MEM_READ_WRITE, 
                                             sizeof(cl::unit_S0) * 500000, nullptr);
    cl_mem s1_mem = cl_program->createBuffer(CL_MEM_READ_WRITE,
                                             sizeof(cl::unit_S1) * 100000, nullptr);

    cl::unit_S0 unit;
    unit.orig_id = 0;
    unit.depth = 0;
    unit.strength.s[0] = unit.strength.s[1] = unit.strength.s[2] = 1;
    unit.start_p.s[0] = 47.870575; unit.start_p.s[1] = 548.800049; unit.start_p.s[2] = 232.352966;
    unit.in_dir.s[0] = -0.228340; unit.in_dir.s[1] = -0.747359; unit.in_dir.s[2] = 0.623952;

    cl_program->writeBuffer(s0_mem, sizeof(unit), &unit);

    cl_int v_sizes[10] = {0};
    v_sizes[0] = 1;
    cl_program->writeBuffer(v_sizes_mem, sizeof(v_sizes), v_sizes);

    // run S0
    auto kernel = cl_program->getKernel("naive_intersect");
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &v_sizes_mem);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &s0_mem);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &s1_mem);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &points_mem);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &geometries_mem);
    clSetKernelArg(kernel, 5, sizeof(cl_int), &geometries_size);

    size_t local = 200;
    size_t global = (v_sizes[0] / local + 1) * local;
    clEnqueueNDRangeKernel(cl_program->commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
    clFinish(cl_program->commands);

    cl_program->readBuffer(v_sizes_mem, sizeof(cl_int) * 10, v_sizes);

    cl::unit_S1 s1;
    cl_program->readBuffer(s1_mem, sizeof(s1), &s1);

    __builtin_trap();

}
