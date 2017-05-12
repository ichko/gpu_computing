#pragma once

#include <CL/cl.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>


struct CLContext {

    cl::Device device;
    cl::Context context;
    cl::Program program;
    cl::Kernel kernel;
    cl::CommandQueue queue;
    std::map<size_t, cl::Buffer> buffers;

    CLContext& run(size_t thread_cnt) {
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(thread_cnt), cl::NullRange);
        queue.finish();

        return *this;
    }

    template <typename T> CLContext& read_buffer(size_t arg_id, T* out, size_t size) {
        queue.enqueueReadBuffer(buffers[arg_id], CL_TRUE, 0, sizeof(T) * size, out);
        return *this;
    }

    template <typename T> CLContext set_arg(
        cl_uint arg_id,
        T* data,
        size_t size = 1,
        int buffer_type = CL_MEM_READ_WRITE
    ) {
        cl::Buffer buffer(context, buffer_type, sizeof(T) * size);
        queue.enqueueWriteBuffer(buffer, CL_TRUE, 0, sizeof(T) * size, data);
        kernel.setArg(arg_id, buffer);
        buffers[arg_id] = buffer;

        return *this;
    }

    CLContext& load_kernel(std::string function_name) {
        kernel = cl::Kernel(program, function_name.c_str());
        return *this;
    }

    CLContext& load_src(std::string src_file) {
        cl::Program::Sources sources;

        std::string kernel_code = read_file(src_file);
        sources.push_back({ kernel_code.c_str(), kernel_code.length() });

        program = cl::Program(context, sources);
        if (program.build({ device }) != CL_SUCCESS) {
            std::cout << " Error building kernel: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << "\n";
            exit(1);
        }

        return *this;
    }

    CLContext& load_device(size_t platform_id = 0, size_t device_id = 0) {
        //get all platforms (drivers)
        std::vector<cl::Platform> all_platforms;
        cl::Platform::get(&all_platforms);
        if (all_platforms.size() == 0) {
            std::cout << " No platforms found. Check OpenCL installation!\n";
            exit(1);
        }
        cl::Platform platform = all_platforms[platform_id];
        std::cout << "Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << "\n";

        std::vector<cl::Device> all_devices;
        platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
        if (all_devices.size() == 0) {
            std::cout << " No devices found. Check OpenCL installation!\n";
            exit(1);
        }
        device = all_devices[device_id];
        std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << "\n";
        context = cl::Context({ device });
        queue = cl::CommandQueue(context, device);

        return *this;
    }

    static std::string read_file(std::string file_name) {
        std::ifstream file;
        file.open(file_name);
        std::stringstream str_stream;
        str_stream << file.rdbuf();
        file.close();

        return str_stream.str();
    }

};
