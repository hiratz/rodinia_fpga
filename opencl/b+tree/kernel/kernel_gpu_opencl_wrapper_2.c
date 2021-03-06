// #ifdef __cplusplus
// extern "C" {
// #endif

//========================================================================================================================================================================================================200
//	INCLUDE
//========================================================================================================================================================================================================200

//======================================================================================================================================================150
//	LIBRARIES
//======================================================================================================================================================150

#include <string.h>									// (in directory known to compiler)			needed by memset
#include <stdio.h>									// (in directory known to compiler)			needed by printf, stderr
#include <assert.h>
//======================================================================================================================================================150
//	COMMON
//======================================================================================================================================================150

#include "../common.h"									// (in directory provided here)

//======================================================================================================================================================150
//	UTILITIES
//======================================================================================================================================================150

//#include "../util/timer/timer.h"						// (in directory provided here)
#include "../../common/timer.h"
//======================================================================================================================================================150
//	HEADER
//======================================================================================================================================================150

#include "./kernel_gpu_opencl_wrapper_2.h"				// (in directory provided here)

#include "../util/opencl/opencl.h"

#include "../common/opencl_util.h"
//========================================================================================================================================================================================================200
//	FUNCTION
//========================================================================================================================================================================================================200

void 
kernel_gpu_opencl_wrapper_2(knode *knodes,
							cl_long knodes_elem,
							long knodes_mem,

							int order,
							cl_long maxheight,
							int count,

							cl_long *currKnode,
							cl_long *offset,
							cl_long *lastKnode,
							cl_long *offset_2,
							int *start,
							int *end,
							int *recstart,
                            int *reclength,
                            cl_context context,
                            int version)
{

	//======================================================================================================================================================150
	//	CPU VARIABLES
	//======================================================================================================================================================150

	// timer
	TimeStamp time0;
	TimeStamp time1;
	TimeStamp time2;
	TimeStamp time3;
	TimeStamp time4;
	TimeStamp time5;
	TimeStamp time6;

        char pbuf[100];

	GetTime(time0);

	//======================================================================================================================================================150
	//	GPU SETUP
	//======================================================================================================================================================150

	//====================================================================================================100
	//	INITIAL DRIVER OVERHEAD
	//====================================================================================================100

	// cudaThreadSynchronize();

	//====================================================================================================100
	//	COMMON VARIABLES
	//====================================================================================================100

	// common variables
	cl_int error;

#if 0        
	//====================================================================================================100
	//	GET PLATFORMS (Intel, AMD, NVIDIA, based on provided library), SELECT ONE
	//====================================================================================================100

	// Get the number of available platforms
	cl_uint num_platforms;
	error = clGetPlatformIDs(	0, 
								NULL, 
								&num_platforms);
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	// Get the list of available platforms
	cl_platform_id *platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms);
	error = clGetPlatformIDs(	num_platforms, 
								platforms, 
								NULL);
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	// Select the 1st platform
	cl_platform_id platform = platforms[0];

	// Get the name of the selected platform and print it (if there are multiple platforms, choose the first one)
	char pbuf[100];
	error = clGetPlatformInfo(	platform, 
								CL_PLATFORM_VENDOR, 
								sizeof(pbuf), 
								pbuf, 
								NULL);
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);
	printf("Platform: %s\n", pbuf);

	//====================================================================================================100
	//	CREATE CONTEXT FOR THE PLATFORM
	//====================================================================================================100

	// Create context properties for selected platform
	cl_context_properties context_properties[3] = {	CL_CONTEXT_PLATFORM, 
													(cl_context_properties) platform, 
													0};

	// Create context for selected platform being GPU
	cl_context context;
	context = clCreateContextFromType(	context_properties, 
										CL_DEVICE_TYPE_ALL, 
										NULL, 
										NULL, 
										&error);
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

#endif
        
	//====================================================================================================100
	//	GET DEVICES AVAILABLE FOR THE CONTEXT, SELECT ONE
	//====================================================================================================100

	// Get the number of devices (previousely selected for the context)
	size_t devices_size;
	error = clGetContextInfo(	context, 
								CL_CONTEXT_DEVICES, 
								0, 
								NULL, 
								&devices_size);
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	// Get the list of devices (previousely selected for the context)
	cl_device_id *devices = (cl_device_id *) malloc(devices_size);
	error = clGetContextInfo(	context, 
								CL_CONTEXT_DEVICES, 
								devices_size, 
								devices, 
								NULL);
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	// Select the first device (previousely selected for the context) (if there are multiple devices, choose the first one)
	cl_device_id device;
	device = devices[0];

	// Get the name of the selected device (previousely selected for the context) and print it
	error = clGetDeviceInfo(device, 
							CL_DEVICE_NAME, 
							sizeof(pbuf), 
							pbuf, 
							NULL);
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);
	printf("Device: %s\n", pbuf);

	//====================================================================================================100
	//	CREATE COMMAND QUEUE FOR THE DEVICE
	//====================================================================================================100

	// Create a command queue
	cl_command_queue command_queue;
	command_queue = clCreateCommandQueue(	context, 
											device, 
											0, 
											&error);
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//====================================================================================================100
	//	CREATE PROGRAM, COMPILE IT
	//====================================================================================================100

	// Load kernel source code from file
	size_t sourceSize = 0;
        // sourceSize was originally determined by strlen, which
        // doesn't work for binary files
        char *kernel_file_path = getVersionedKernelName(
            "./kernel/kernel_gpu_opencl_altera", version);
        fprintf(stderr, "kernel file path: '%s'\n",
                kernel_file_path);
	char *source = read_kernel(kernel_file_path, &sourceSize);
        free(kernel_file_path);

	// Create the program
#if defined(USE_JIT)        
	cl_program program = clCreateProgramWithSource(	context, 
							1, 
							(const char**) &source, 
							&sourceSize, 
							&error);
#else
	cl_program program = clCreateProgramWithBinary(	context,
                                                        1,
                                                        devices,
                                                        &sourceSize,
                                                        (const unsigned char**)&source,
                                                        NULL,
                                                        &error);
#endif
        
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	fprintf(stderr, "createProgram succeeded\n");
	free(source);
          
	char clOptions[110];
	//  sprintf(clOptions,"-I../../src");                                                                                 
	sprintf(clOptions,"-I./../ -Ikernel");

#ifdef DEFAULT_ORDER_2
	sprintf(clOptions + strlen(clOptions), " -DDEFAULT_ORDER_2=%d", DEFAULT_ORDER_2);
#endif

	// Compile the program
	error = clBuildProgram(	program, 
							1, 
							&device, 
							clOptions, 
							NULL, 
							NULL);
	// Print warnings and errors from compilation
	static char log[65536]; 
	memset(log, 0, sizeof(log));
	clGetProgramBuildInfo(	program, 
							device, 
							CL_PROGRAM_BUILD_LOG, 
							sizeof(log)-1, 
							log, 
							NULL);
	printf("-----OpenCL Compiler Output-----\n");
	if (strstr(log,"warning:") || strstr(log, "error:")) 
		printf("<<<<\n%s\n>>>>\n", log);
	printf("--------------------------------\n");
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	// Create kernel
	cl_kernel kernel;
	kernel = clCreateKernel(program, 
							"findRangeK", 
							&error);
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	GetTime(time1);

	//====================================================================================================100
	//	END
	//====================================================================================================100

	//======================================================================================================================================================150
	//	GPU MEMORY				MALLOC
	//======================================================================================================================================================150

	//====================================================================================================100
	//	DEVICE IN
	//====================================================================================================100

	//==================================================50
	//	knodesD
	//==================================================50

	cl_mem knodesD;
	knodesD = clCreateBuffer(	context, 
								CL_MEM_READ_WRITE, 
								knodes_mem, 
								NULL, 
								&error );
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	currKnodeD
	//==================================================50

	cl_mem currKnodeD;
	currKnodeD = clCreateBuffer(context, 
								CL_MEM_READ_WRITE, 
								count*sizeof(cl_long), 
								NULL, 
								&error );
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	offsetD
	//==================================================50

	cl_mem offsetD;
	offsetD = clCreateBuffer(	context, 
								CL_MEM_READ_WRITE, 
								count*sizeof(cl_long), 
								NULL, 
								&error );
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	lastKnodeD
	//==================================================50

	cl_mem lastKnodeD;
	lastKnodeD = clCreateBuffer(context, 
								CL_MEM_READ_WRITE, 
								count*sizeof(cl_long), 
								NULL, 
								&error );
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	offset_2D
	//==================================================50

	cl_mem offset_2D;
	offset_2D = clCreateBuffer(context, 
								CL_MEM_READ_WRITE, 
								count*sizeof(cl_long), 
								NULL, 
								&error );
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	startD
	//==================================================50

	cl_mem startD;
	startD = clCreateBuffer(context, 
								CL_MEM_READ_WRITE, 
								count*sizeof(int), 
								NULL, 
								&error );
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	endD
	//==================================================50

	cl_mem endD;
	endD = clCreateBuffer(	context, 
							CL_MEM_READ_WRITE, 
							count*sizeof(int), 
							NULL, 
							&error );
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	END
	//==================================================50

	//====================================================================================================100
	//	DEVICE IN/OUT
	//====================================================================================================100

	//==================================================50
	//	ansDStart
	//==================================================50

	cl_mem ansDStart;
	ansDStart = clCreateBuffer(	context, 
							CL_MEM_READ_WRITE, 
							count*sizeof(int), 
							NULL, 
							&error );
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	ansDLength
	//==================================================50

	cl_mem ansDLength;
	ansDLength = clCreateBuffer(	context, 
							CL_MEM_READ_WRITE, 
							count*sizeof(int), 
							NULL, 
							&error );
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	GetTime(time2);

	//==================================================50
	//	END
	//==================================================50

	//====================================================================================================100
	//	END
	//====================================================================================================100

	//======================================================================================================================================================150
	//	GPU MEMORY			COPY
	//======================================================================================================================================================150

	//====================================================================================================100
	//	DEVICE IN
	//====================================================================================================100

	//==================================================50
	//	knodesD
	//==================================================50

	error = clEnqueueWriteBuffer(	command_queue,			// command queue
									knodesD,				// destination
									1,						// block the source from access until this copy operation complates (1=yes, 0=no)
									0,						// offset in destination to write to
									knodes_mem,				// size to be copied
									knodes,					// source
									0,						// # of events in the list of events to wait for
									NULL,					// list of events to wait for
									NULL);					// ID of this operation to be used by waiting operations
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	currKnodeD
	//==================================================50

	error = clEnqueueWriteBuffer(	command_queue,			// command queue
									currKnodeD,				// destination
									1,						// block the source from access until this copy operation complates (1=yes, 0=no)
									0,						// offset in destination to write to
									count*sizeof(cl_long),		// size to be copied
									currKnode,				// source
									0,						// # of events in the list of events to wait for
									NULL,					// list of events to wait for
									NULL);					// ID of this operation to be used by waiting operations
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	offsetD
	//==================================================50

	error = clEnqueueWriteBuffer(	command_queue,			// command queue
									offsetD,				// destination
									1,						// block the source from access until this copy operation complates (1=yes, 0=no)
									0,						// offset in destination to write to
									count*sizeof(cl_long),		// size to be copied
									offset,					// source
									0,						// # of events in the list of events to wait for
									NULL,					// list of events to wait for
									NULL);					// ID of this operation to be used by waiting operations
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	lastKnodeD
	//==================================================50

	error = clEnqueueWriteBuffer(	command_queue,			// command queue
									lastKnodeD,				// destination
									1,						// block the source from access until this copy operation complates (1=yes, 0=no)
									0,						// offset in destination to write to
									count*sizeof(cl_long),		// size to be copied
									lastKnode,				// source
									0,						// # of events in the list of events to wait for
									NULL,					// list of events to wait for
									NULL);					// ID of this operation to be used by waiting operations
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	offset_2D
	//==================================================50

	error = clEnqueueWriteBuffer(	command_queue,			// command queue
									offset_2D,				// destination
									1,						// block the source from access until this copy operation complates (1=yes, 0=no)
									0,						// offset in destination to write to
									count*sizeof(cl_long),		// size to be copied
									offset_2,				// source
									0,						// # of events in the list of events to wait for
									NULL,					// list of events to wait for
									NULL);					// ID of this operation to be used by waiting operations
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	startD
	//==================================================50

	error = clEnqueueWriteBuffer(	command_queue,			// command queue
									startD,					// destination
									1,						// block the source from access until this copy operation complates (1=yes, 0=no)
									0,						// offset in destination to write to
									count*sizeof(int),		// size to be copied
									start,					// source
									0,						// # of events in the list of events to wait for
									NULL,					// list of events to wait for
									NULL);					// ID of this operation to be used by waiting operations
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	endD
	//==================================================50

	error = clEnqueueWriteBuffer(	command_queue,			// command queue
									endD,					// destination
									1,						// block the source from access until this copy operation complates (1=yes, 0=no)
									0,						// offset in destination to write to
									count*sizeof(int),		// size to be copied
									end,					// source
									0,						// # of events in the list of events to wait for
									NULL,					// list of events to wait for
									NULL);					// ID of this operation to be used by waiting operations
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	END
	//==================================================50

	//====================================================================================================100
	//	DEVICE IN/OUT
	//====================================================================================================100

	//==================================================50
	//	ansDStart
	//==================================================50

	error = clEnqueueWriteBuffer(	command_queue,			// command queue
									endD,					// destination
									1,						// block the source from access until this copy operation complates (1=yes, 0=no)
									0,						// offset in destination to write to
									count*sizeof(int),		// size to be copied
									end,					// source
									0,						// # of events in the list of events to wait for
									NULL,					// list of events to wait for
									NULL);					// ID of this operation to be used by waiting operations
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	ansDLength
	//==================================================50

	error = clEnqueueWriteBuffer(	command_queue,			// command queue
									ansDLength,					// destination
									1,						// block the source from access until this copy operation complates (1=yes, 0=no)
									0,						// offset in destination to write to
									count*sizeof(int),		// size to be copied
									reclength,					// source
									0,						// # of events in the list of events to wait for
									NULL,					// list of events to wait for
									NULL);					// ID of this operation to be used by waiting operations
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	GetTime(time3);

	//==================================================50
	//	END
	//==================================================50

	//======================================================================================================================================================150
	//	KERNEL
	//======================================================================================================================================================150

	//====================================================================================================100
	//	Execution Parameters
	//====================================================================================================100

	size_t local_work_size[1];
	local_work_size[0] = order < 1024 ? order : 1024;
	size_t global_work_size[1];
	global_work_size[0] = count * local_work_size[0];

	printf("# of blocks = %d, # of threads/block = %d (ensure that device can handle)\n", (int)(global_work_size[0]/local_work_size[0]), (int)local_work_size[0]);

	//====================================================================================================100
	//	Kernel Arguments
	//====================================================================================================100

	CL_SAFE_CALL( clSetKernelArg( kernel, 0, sizeof(cl_long), (void *) &maxheight) );
	CL_SAFE_CALL( clSetKernelArg( kernel, 1, sizeof(cl_mem), (void *) &knodesD) );
	CL_SAFE_CALL( clSetKernelArg( kernel, 2, sizeof(cl_long), (void *) &knodes_elem) );
	CL_SAFE_CALL( clSetKernelArg( kernel, 3, sizeof(cl_mem), (void *) &currKnodeD) );
	CL_SAFE_CALL( clSetKernelArg( kernel, 4, sizeof(cl_mem), (void *) &offsetD) );
	CL_SAFE_CALL( clSetKernelArg( kernel, 5, sizeof(cl_mem), (void *) &lastKnodeD) );
	CL_SAFE_CALL( clSetKernelArg( kernel, 6, sizeof(cl_mem), (void *) &offset_2D) );
	CL_SAFE_CALL( clSetKernelArg( kernel, 7, sizeof(cl_mem), (void *) &startD) );
	CL_SAFE_CALL( clSetKernelArg( kernel, 8, sizeof(cl_mem), (void *) &endD) );
	CL_SAFE_CALL( clSetKernelArg( kernel, 9, sizeof(cl_mem), (void *) &ansDStart) );
	CL_SAFE_CALL( clSetKernelArg( kernel, 10, sizeof(cl_mem), (void *) &ansDLength) );

        if (!is_ndrange_kernel(version)) {
		CL_SAFE_CALL( clSetKernelArg(kernel, 11, sizeof(count), (void *)&count) );
		CL_SAFE_CALL( clSetKernelArg(kernel, 12, sizeof(order), (void *)&order) );
        }


	//====================================================================================================100
	//	Kernel
	//====================================================================================================100

        if (is_ndrange_kernel(version)) {
          error = clEnqueueNDRangeKernel(	command_queue, 
                                                kernel, 
                                                1, 
                                                NULL, 
                                                global_work_size, 
                                                local_work_size, 
                                                0, 
                                                NULL, 
                                                NULL);
        } else {
          error = clEnqueueTask(command_queue, kernel, 0, 
                                NULL, NULL);
        }
                                                         
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	// Wait for all operations to finish NOT SURE WHERE THIS SHOULD GO
	error = clFinish(command_queue);
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	GetTime(time4);

	//====================================================================================================100
	//	END
	//====================================================================================================100

	//======================================================================================================================================================150
	//	GPU MEMORY			COPY (CONTD.)
	//======================================================================================================================================================150

	//====================================================================================================100
	//	DEVICE IN/OUT
	//====================================================================================================100

	//==================================================50
	//	ansDStart
	//==================================================50

	error = clEnqueueReadBuffer(command_queue,				// The command queue.
								ansDStart,					// The image on the device.
								CL_TRUE,					// Blocking? (ie. Wait at this line until read has finished?)
								0,							// Offset. None in this case.
								count*sizeof(int),			// Size to copy.
								recstart,					// The pointer to the image on the host.
								0,							// Number of events in wait list. Not used.
								NULL,						// Event wait list. Not used.
								NULL);						// Event object for determining status. Not used.
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	//==================================================50
	//	ansDLength
	//==================================================50

	error = clEnqueueReadBuffer(command_queue,				// The command queue.
								ansDLength,					// The image on the device.
								CL_TRUE,					// Blocking? (ie. Wait at this line until read has finished?)
								0,							// Offset. None in this case.
								count*sizeof(int),			// Size to copy.
								reclength,					// The pointer to the image on the host.
								0,							// Number of events in wait list. Not used.
								NULL,						// Event wait list. Not used.
								NULL);						// Event object for determining status. Not used.
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	GetTime(time5);

	//==================================================50
	//	END
	//==================================================50

	//====================================================================================================100
	//	END
	//====================================================================================================100

	//======================================================================================================================================================150
	//	GPU MEMORY DEALLOCATION
	//======================================================================================================================================================150

	// Release kernels...
	clReleaseKernel(kernel);

	// Now the program...
	clReleaseProgram(program);

	// Clean up the device memory...
	clReleaseMemObject(knodesD);

	clReleaseMemObject(currKnodeD);
	clReleaseMemObject(offsetD);
	clReleaseMemObject(lastKnodeD);
	clReleaseMemObject(offset_2D);
	clReleaseMemObject(startD);
	clReleaseMemObject(endD);
	clReleaseMemObject(ansDStart);
	clReleaseMemObject(ansDLength);

	// Flush the queue
	error = clFlush(command_queue);
	if (error != CL_SUCCESS) 
		fatal_CL(error, __LINE__);

	// ...and finally, the queue and context.
	clReleaseCommandQueue(command_queue);

	// ???
	//clReleaseContext(context);

	GetTime(time6);

	//======================================================================================================================================================150
	//	DISPLAY TIMING
	//======================================================================================================================================================150

	printf("Time spent in different stages of GPU_CUDA KERNEL:\n");
	printf("%15.12f s, %15.12f %% : GPU: SET DEVICE / DRIVER INIT\n",
               TimeDiff(time0, time1) / 1000,
               TimeDiff(time0, time1) / TimeDiff(time0, time6) * 100);
	printf("%15.12f s, %15.12f %% : GPU MEM: ALO\n",
               TimeDiff(time1, time2) / 1000,
               TimeDiff(time1, time2) / TimeDiff(time0, time6) * 100);
	printf("%15.12f s, %15.12f %% : GPU MEM: COPY IN\n",
               TimeDiff(time2, time3) / 1000,
               TimeDiff(time2, time3) / TimeDiff(time0, time6) * 100);
	printf("%15.12f s, %15.12f %% : GPU: KERNEL\n",
               TimeDiff(time3, time4) / 1000,
               TimeDiff(time3, time4) / TimeDiff(time0, time6) * 100);
	printf("%15.12f s, %15.12f %% : GPU MEM: COPY OUT\n",
               TimeDiff(time4, time5) / 1000,
               TimeDiff(time4, time5) / TimeDiff(time0, time6) * 100);
	printf("%15.12f s, %15.12f %% : GPU MEM: FRE\n",
               TimeDiff(time5, time6) / 1000,
               TimeDiff(time5, time6) / TimeDiff(time0, time6) * 100);
	printf("Total time:\n");
	printf("%.12f s\n", TimeDiff(time0, time6) / 1000);

	//======================================================================================================================================================150
	//	END
	//======================================================================================================================================================150

}

//========================================================================================================================================================================================================200
//	END
//========================================================================================================================================================================================================200

// #ifdef __cplusplus
// }
// #endif
