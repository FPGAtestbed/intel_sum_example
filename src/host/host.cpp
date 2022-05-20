#define CL_TARGET_OPENCL_VERSION 200

#include "AOCLUtils/aocl_utils.h"
#include "CL/cl_ext_intelfpga.h"
#include "CL/opencl.h"
#include "aocl_mmd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "AOCLUtils/aocl_utils.h"

using namespace aocl_utils;

// Kernel name provided in the CL file
const char *kernel_name = "sum_kernel";  

// OpenCL runtime configuration
static cl_platform_id platform = NULL;
static cl_device_id device = NULL;
static cl_context context = NULL;
static cl_command_queue queue = NULL;
static cl_kernel kernel = NULL;
static cl_program program = NULL;

static cl_mem input_data, output_data; 
scoped_aligned_ptr<double> input_data_host, output_data_host;

// Function prototypes
bool init_device();
bool init_memory(int);
bool schedule_kernel(int, double);
void display_results(int);
void cleanup();

// Entry point.
int main(int argc, char * argv[]) {
  cl_int status;

  if (argc != 2) {
    fprintf(stderr, "You must provide the number of elements to sum as a command line argument\n");
    return -1;
  }

  int number_els=atoi(argv[1]);

  if(!init_device()) return -1;
  printf("Kernel initialization is complete\n");

  if(!init_memory(number_els)) return -1;
  printf("Memory initialization is complete\n");

  if (!schedule_kernel(number_els, 100)) return -1;
  printf("Kernel execution has been scheduled\n");

  // Wait for command queue to complete pending events
  status = clFinish(queue);
  checkError(status, "Failed to finish execution based on schedule");

  printf("Kernel execution has completed successfully\n");

  display_results(number_els);

  // Free the resources allocated
  cleanup();

  return 0;
}

void display_results(int number_els) {
  printf("============================\nDisplaying results\n============================\n");
  for (int i=0;i<number_els;i++) {
    printf("%d: %.2f\n", i, output_data_host[i]);
  }
}

bool schedule_kernel(int number_els, double number_to_add) {
  cl_int status;
  cl_event copy_on_event, kernel_event, copy_off_event;

  // Set the kernel arguments
  status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_data);
  checkError(status, "Failed to set kernel arg 0");
  status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_data);
  checkError(status, "Failed to set kernel arg 1");
  status = clSetKernelArg(kernel, 2, sizeof(double), &number_to_add);
  checkError(status, "Failed to set kernel arg 2");
  status = clSetKernelArg(kernel, 3, sizeof(int), &number_els);
  checkError(status, "Failed to set kernel arg 3");

  status = clEnqueueWriteBuffer(queue, input_data, CL_FALSE, 0,  number_els * sizeof(double), input_data_host, 0, NULL, &copy_on_event);
  checkError(status, "Failed to schedule transfer of input data");
  status = clEnqueueTask(queue, kernel, 1, &copy_on_event, &kernel_event);
  checkError(status, "Failed to launch read_data_kernel");
  status = clEnqueueReadBuffer(queue, output_data, CL_FALSE, 0, number_els * sizeof(double), output_data_host, 1, &kernel_event, &copy_off_event);
  checkError(status, "Failed to transfer output su_data");

  return true;
}

bool init_memory(int number_els) {
  cl_int status;

  input_data = clCreateBuffer(context, CL_MEM_READ_ONLY, number_els * sizeof(double), NULL, &status);
  checkError(status, "Failed to create buffer for input data");
  output_data = clCreateBuffer(context, CL_MEM_WRITE_ONLY, number_els * sizeof(double), NULL, &status);
  checkError(status, "Failed to create buffer for output data");  

  input_data_host.reset(number_els);
  output_data_host.reset(number_els);

  for (int i=0;i<number_els;i++) {
    input_data_host[i]=i;
  }

  return true;
}

bool init_device() {
  cl_int status;

  if(!setCwdToExeDir()) {
    return false;
  }

  // Get the OpenCL platform.
  if(getenv("CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA")) {
    platform = findPlatform("Intel(R) FPGA Emulation Platform for OpenCL(TM)");
    if (platform == NULL) {
      fprintf(stderr, "ERROR: Unable to find Intel emulation platform\n");
      return false;
    }
    printf("Execution mode: Emulation\n");
  } else {
    platform = findPlatform("Intel(R) FPGA SDK for OpenCL(TM)");
    if (platform == NULL) {
      fprintf(stderr, "ERROR: Unable to find Intel platform\n");
      return false;
    }
    printf("Execution mode: Hardware\n");
  }

  // Query the available OpenCL devices.
  scoped_array<cl_device_id> devices;
  cl_uint num_devices;

  devices.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));

  // We'll just use the first device.
  device = devices[0];

  // Create the context.
  context = clCreateContext(NULL, 1, &device, &oclContextCallback, NULL, &status);
  checkError(status, "Failed to create context");

  // Create the command queue.
  queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &status);
  checkError(status, "Failed to create command queue");

  // Create the program.
  std::string binary_file = getBoardBinaryFile("device", device);
  printf("Using AOCX: %s\n", binary_file.c_str());
  program = createProgramFromBinary(context, binary_file.c_str(), &device, 1);

  // Build the program
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");

  // Create the kernel
  kernel = clCreateKernel(program, kernel_name, &status);
  checkError(status, "Failed to create kernel");

  return true;
}

void cleanup() {
  if (input_data) clReleaseMemObject(input_data);
  if (output_data) clReleaseMemObject(output_data);
  if(kernel) clReleaseKernel(kernel);    
  if(program) clReleaseProgram(program);
  if(queue) clReleaseCommandQueue(queue);
  if(context) clReleaseContext(context);
}

