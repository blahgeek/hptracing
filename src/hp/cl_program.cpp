/* 
* @Author: BlahGeek
* @Date:   2015-01-10
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-01-10
*/

#include "./cl_program.h"
#include "./common.h"
#include <string>
#include <fstream>

static std::string read_file(std::string filename) {
    std::ifstream t(filename.c_str());
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    return str;
}

hp::CLProgram::CLProgram() {
    std::string source = read_file("src/hp/unit/types.h.cl") + 
                         read_file("src/hp/unit/kernel.cl");

    auto err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    hp_assert(err == CL_SUCCESS);

    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    hp_assert(err == CL_SUCCESS);

    commands = clCreateCommandQueue(context, device_id, 0, &err);
    hp_assert(err == CL_SUCCESS);

    const char * source_s = source.c_str();
    program = clCreateProgramWithSource(context, 1, &source_s, NULL, &err);
    hp_assert(err == CL_SUCCESS);

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if(err != CL_SUCCESS) {
        size_t len; char buffer[20480];
        hp_log("Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        hp_log("%s\n", buffer);
        hp_assert(false);
    }

}

cl_kernel hp::CLProgram::getKernel(const char * kernel_name) {
    cl_int err;
    cl_kernel kernel = clCreateKernel(program, kernel_name, &err);
    hp_assert(err == CL_SUCCESS);
    return kernel;
}

cl_mem hp::CLProgram::createBuffer(cl_mem_flags flags, size_t len, void * host_p) {
    cl_int err;
    cl_mem buffer = clCreateBuffer(context, flags, len, host_p, &err);
    hp_assert(err == CL_SUCCESS);
    return buffer;
}

hp::CLProgram::~CLProgram() {
    clReleaseProgram(program);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);
}

void hp::CLProgram::readBuffer(cl_mem buffer, size_t len, void * p) {
    auto err = clEnqueueReadBuffer(commands, buffer, CL_TRUE, 0, len, p, 0, NULL, NULL);
    hp_assert(err == CL_SUCCESS);
}

void hp::CLProgram::writeBuffer(cl_mem buffer, size_t len, void * p) {
    auto err = clEnqueueWriteBuffer(commands, buffer, CL_TRUE, 0, len, p, 0, NULL, NULL);
    hp_assert(err == CL_SUCCESS);
}
