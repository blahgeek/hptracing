/* 
* @Author: BlahGeek
* @Date:   2015-01-09
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-10
*/

#include <iostream>
#include <gtest/gtest.h>
#include <fstream>

#include "./hp/common.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

typedef struct {
    cl_int orig_id;
    cl_int depth;
    cl_float3 strength;
    cl_float3 start_p, in_dir;
} unit_S0;

typedef struct {
    cl_int orig_id;
    cl_int depth;
    cl_float3 strength;
    cl_int geo_id;
    cl_int mat_id;
    cl_float3 start_p, in_dir;
    cl_float intersect_number;
} unit_S1;

#include <sys/time.h>

uint64_t GetTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

std::string read_file(std::string filename) {
    std::ifstream t(filename.c_str());
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    return str;
}

TEST(CLTest, naive_intersect) {
    std::string source = read_file("src/hp/unit/types.h.cl") + 
                         read_file("src/hp/unit/naive_intersect.cl");
    // std::cerr << "Source length: " << source.length() << std::endl;
    cl_device_id device_id;
    auto err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    EXPECT_EQ(err, CL_SUCCESS);
    cl_context context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    cl_command_queue commands = clCreateCommandQueue(context, device_id, 0, &err);
    const char * tmp = source.c_str();
    cl_program program = clCreateProgramWithSource(context, 1, 
                                                   &tmp, NULL, &err);
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];

        printf("Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        exit(1);
    }

    cl_kernel kernel = clCreateKernel(program, "naive_intersect", &err);

    EXPECT_EQ(err, CL_SUCCESS);
    EXPECT_TRUE(kernel);

    cl_float scene_points[] = {0.0, 0.0, 0.0, 0.0,
                               1000.0, 0.0, 0.0, 0.0,
                               0.0, 1000.0, 0.0, 0.0};
    cl_int scene_mesh[] = {0, 1, 2, 0};

    cl_int scene_mesh_size = 1;

    unit_S0 * v_s0 = new unit_S0[1000000];
    // unit_S1 v_s1[1000000];
    cl_int v_s1_size = 0;

    for(int i = 0 ; i < 1000 ; i += 1) {
        for(int j = 0 ; j < 1000 ; j += 1) {
            hp::Vec p(i - 500, j - 500, 500); p.normalize();
            int index = i * 1000 + j;
            v_s0[index].orig_id = index;
            v_s0[index].start_p.s[0] = 500;
            v_s0[index].start_p.s[1] = 500;
            v_s0[index].start_p.s[2] = -500;
            v_s0[index].in_dir.s[0] = p[0];
            v_s0[index].in_dir.s[1] = p[1];
            v_s0[index].in_dir.s[2] = p[2];
        }
    }

    cl_mem s0_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                   sizeof(unit_S0) * 1000000, v_s0, &err);
    cl_mem s1_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY , 
                                   sizeof(unit_S1) * 1000000, NULL, &err);
    cl_mem s1_size_mem = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,  
                                        sizeof(cl_int), &(v_s1_size), &err);
    cl_mem scene_points_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                             sizeof(scene_points), scene_points, &err);
    cl_mem scene_mesh_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                           sizeof(scene_mesh), scene_mesh, &err);

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &s0_mem);
    EXPECT_EQ(err, CL_SUCCESS);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &s1_mem);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &s1_size_mem);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &scene_points_mem);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &scene_mesh_mem);
    clSetKernelArg(kernel, 5, sizeof(cl_int), &scene_mesh_size);

    auto t0 = GetTimeStamp();

    size_t global = 1000000;
    size_t local = 200;
    err = clEnqueueNDRangeKernel(commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
    EXPECT_EQ(err, CL_SUCCESS);
    clFinish(commands);

    // clEnqueueReadBuffer(commands, s1_mem, CL_TRUE, 0, sizeof(unit_S1) * 10000, v_s1, 0, NULL, NULL);
    err = clEnqueueReadBuffer(commands, s1_size_mem, CL_TRUE, 0, sizeof(cl_int), &(v_s1_size), 0, NULL, NULL);
    auto t1 = GetTimeStamp();

    EXPECT_EQ(err, CL_SUCCESS);

    std::cout << "TIME: " << t1 - t0 << std::endl;
    std::cout << "SIZE: " << v_s1_size << std::endl;
    EXPECT_TRUE(std::abs(v_s1_size - 500000) < 1000);

}
