#pragma once

#include "ofMain.h"
#include <assert.h>
#include <cl/Opencl.h>
#include "MSAOpenCLKernel.h"
#include "MSAOpenCLProgram.h"
#include "MSAOpenCLBuffer.h"
#include "MSAOpenCLTypes.h"
#include "MSAOpenCLImage.h"
//#include "MSAOpenCLImagePingPong.h"

namespace msa {
	
	class OpenCL {
	public:
		OpenCL();
		~OpenCL();
		
		static OpenCL *currentOpenCL;
		
		// initializes openCL with the passed in device (leave empty for default)
		bool	setup(int clDeviceType = CL_DEVICE_TYPE_GPU);
		bool	setupFromOpenGL();
		
		bool				isInitialised() const	{	return isSetup && mDevices.size() && mQueues.size();	}
		cl_device_id&		getDevice();
		cl_context&			getContext();
		cl_command_queue&	getQueue();
		cl_command_queue	createNewQueue();	//	create extra queue
		
		
		// doesn't return until all commands in the queue have been sent
		void	flush();
		
		
		// doesn't return until all commands in the queue have been sent and finished executing
		void	finish();	
		
		
		// load a program (contains a bunch of kernels)
		// returns pointer to the program should you need it (for most operations you won't need this)
		OpenCLProgram*	loadProgramFromFile(string filename, bool isBinary=false,const char* BuildOptions=NULL);
		OpenCLProgram*	loadProgramFromSource(string programSource);
		
		
		// specify a kernel to load from the specified program
		// returns pointer to the kernel 
		OpenCLKernel*	loadKernel(string kernelName,OpenCLProgram& program,cl_command_queue Queue=NULL);
		void			deleteKernel(OpenCLKernel& Kernel);
		
		
		
		// create OpenCL buffer memory objects
		// if dataPtr parameter is passed in, data is uploaded immediately
		// parameters with default values can be omited
		OpenCLBuffer*	createBuffer(int numberOfBytes,
									 cl_mem_flags memFlags = CL_MEM_READ_WRITE,
									 void *dataPtr = NULL,
									 bool blockingWrite = CL_FALSE,cl_command_queue Queue=NULL);
		
		// create buffer from the GL Object - e.g. VBO (they share memory space on device)
		// parameters with default values can be omited
		OpenCLBuffer*	createBufferFromGLObject(GLuint glBufferObject,
												 cl_mem_flags memFlags = CL_MEM_READ_WRITE);
		
		void			deleteBuffer(OpenCLMemoryObject& Buffer);
		
		
		// create OpenCL image memory objects
		// if dataPtr parameter is passed in, data is uploaded immediately
		
		// create a 2D Image with given properties
		// Image is not linked to an OpenGL texture
		// parameters with default values can be omited
		OpenCLImage*		createImage2D(int width,
										  int height,
										  cl_channel_order imageChannelOrder = CL_RGBA,
										  cl_channel_type imageChannelDataType = CL_FLOAT,
										  cl_mem_flags memFlags = CL_MEM_READ_WRITE,
										  void *dataPtr = NULL,
										  bool blockingWrite = CL_FALSE);
		
		// create a 2D Image from the ofTexture passed in (they share memory space on device)
		// parameters with default values can be omited
		OpenCLImage*		createImageFromTexture(ofTexture &tex,
												   cl_mem_flags memFlags = CL_MEM_READ_WRITE,
												   int mipLevel = 0);
		
		
		// create both a 2D Image AND an ofTexture at the same time (they share memory space on device)
		// parameters with default values can be omited
		OpenCLImage*		createImageWithTexture(int width,
												   int height,
												   int glType = GL_RGBA,
												   cl_mem_flags memFlags = CL_MEM_READ_WRITE);
		
		
		
		// parameters with default values can be omited
		OpenCLImage*		createImage3D(int width,
										  int height,
										  int depth,
										  cl_channel_order imageChannelOrder = CL_RGBA,
										  cl_channel_type imageChannelDataType = CL_FLOAT,
										  cl_mem_flags memFlags = CL_MEM_READ_WRITE,
										  void *dataPtr = NULL,
										  bool blockingWrite = CL_FALSE);
		
		string getInfoAsString();
		static const char*	getErrorAsString(cl_int err);
		
		struct {
			cl_char		vendorName[1024];
			cl_char		deviceName[1024];
			cl_char		driverVersion[1024];
			cl_char		deviceVersion[1024];
			cl_uint		maxComputeUnits;
			cl_uint		maxWorkItemDimensions;
			size_t		maxWorkItemSizes[32];
			size_t		maxWorkGroupSize;
			cl_uint		maxClockFrequency;
			cl_ulong	maxMemAllocSize;
			cl_bool		imageSupport;
			cl_uint		maxReadImageArgs;
			cl_uint		maxWriteImageArgs;
			size_t		image2dMaxWidth;
			size_t		image2dMaxHeight;
			size_t		image3dMaxWidth;
			size_t		image3dMaxHeight;
			size_t		image3dMaxDepth;
			cl_uint		maxSamplers;
			size_t		maxParameterSize;
			cl_ulong	globalMemCacheSize;
			cl_ulong	globalMemSize;
			cl_ulong	maxConstantBufferSize;
			cl_uint		maxConstantArgs;
			cl_ulong	localMemSize;
			cl_bool		errorCorrectionSupport;
			size_t		profilingTimerResolution;
			cl_bool		endianLittle;
			cl_char		profile[1024];
			cl_char		extensions[1024];		
		} info;
		
		
	protected:	
		
		vector<cl_device_id>			mDevices;
		cl_context						clContext;
		vector<cl_command_queue>		mQueues;

		ofMutex							mMemObjectsLock;
		ofMutex							mKernelsLock;
		ofMutex							mProgramsLock;

		vector<OpenCLProgram*>			programs;	
		vector<OpenCLKernel*>			kernels;
		vector<OpenCLMemoryObject*>		memObjects;
		bool							isSetup;
		
		bool createDevice(int clDeviceType);
		void createQueue();
	};
	
}