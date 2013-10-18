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
	
	class clDeviceInfo
	{
	public:
		//	gr: essential that some parameters are initialsied
		clDeviceInfo() :
			type	( CL_DEVICE_TYPE_ALL )
		{
		}

	public:
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
		cl_device_type		type;
	};

	class OpenClDevice
	{
	public:
		enum Type
		{
			Invalid = CL_DEVICE_TYPE_ALL,
			Any	= CL_DEVICE_TYPE_CPU|CL_DEVICE_TYPE_GPU,
			CPU	= CL_DEVICE_TYPE_CPU,
			GPU	= CL_DEVICE_TYPE_GPU,
		};
	public:
		OpenClDevice() :
			mPlatform	( NULL ),
			mDeviceId	( NULL )
		{
		}

		Type			GetType() const	{	return static_cast<Type>( mInfo.type );	}

	public:
		cl_platform_id	mPlatform;
		cl_device_id	mDeviceId;
		clDeviceInfo	mInfo;
	};

	class OpenCL {
	public:
		OpenCL();
		~OpenCL();
		
		// initializes openCL with the passed in device (leave empty for default)
		bool	setup();
		bool	setupFromOpenGL();
		
		bool				isInitialised() const	{	return HasDevice() && HasContext();	}
		cl_context			getContext() const		{	return mContext;	}
		cl_command_queue	createQueue(OpenClDevice::Type DeviceType);	//	create extra queue
		bool				HasDevice() const		{	return mDevices.size();	}
		bool				HasContext() const		{	return getContext() != NULL;	}
		OpenClDevice*		GetDevice(OpenClDevice::Type DeviceType);
		OpenClDevice*		GetDevice(cl_command_queue Queue);
		OpenClDevice*		GetDevice(cl_device_id Device);
		template<size_t MAXDEVICES>
		int					GetDevices(cl_device_id (&Devices)[MAXDEVICES]);

		// load a program (contains a bunch of kernels)
		// returns pointer to the program should you need it (for most operations you won't need this)
		OpenCLProgram*	loadProgramFromFile(string filename, bool isBinary=false,const char* BuildOptions=NULL);
		OpenCLProgram*	loadProgramFromSource(string programSource);
		
		
		// specify a kernel to load from the specified program
		// returns pointer to the kernel 
		OpenCLKernel*	loadKernel(string kernelName,OpenCLProgram& program,cl_command_queue Queue);
		void			deleteKernel(OpenCLKernel& Kernel);
		
		
		
		// create OpenCL buffer memory objects
		// if dataPtr parameter is passed in, data is uploaded immediately
		// parameters with default values can be omited
		OpenCLBuffer*	createBuffer(cl_command_queue Queue,int numberOfBytes,
									 cl_mem_flags memFlags = CL_MEM_READ_WRITE,
									 void *dataPtr = NULL,
									 bool blockingWrite = CL_FALSE);
		
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
		OpenCLImage*		createImage2D(cl_command_queue Queue,int width,
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
		OpenCLImage*		createImageWithTexture(cl_command_queue Queue,int width,
												   int height,
												   int glType = GL_RGBA,
												   cl_mem_flags memFlags = CL_MEM_READ_WRITE);
		
		
		
		// parameters with default values can be omited
		OpenCLImage*		createImage3D(cl_command_queue Queue,int width,
										  int height,
										  int depth,
										  cl_channel_order imageChannelOrder = CL_RGBA,
										  cl_channel_type imageChannelDataType = CL_FLOAT,
										  cl_mem_flags memFlags = CL_MEM_READ_WRITE,
										  void *dataPtr = NULL,
										  bool blockingWrite = CL_FALSE);
		
		string				getInfoAsString(const clDeviceInfo& Info);
		static const char*	getErrorAsString(cl_int err);
		
	
	protected:
		bool				createDevices();
	
	protected:	
		
		vector<OpenClDevice>			mDevices;
		cl_context						mContext;
		vector<cl_command_queue>		mQueues;

		ofMutex							mMemObjectsLock;
		ofMutex							mKernelsLock;
		ofMutex							mProgramsLock;
		ofMutex							mQueuesLock;

		vector<OpenCLProgram*>			programs;	
		vector<OpenCLKernel*>			kernels;
		vector<OpenCLMemoryObject*>		memObjects;
	};



	template<size_t MAXDEVICES>
	inline int OpenCL::GetDevices(cl_device_id (&Devices)[MAXDEVICES])
	{
		int DeviceCount = 0;
		for ( int d=0;	DeviceCount<MAXDEVICES && d<mDevices.size();	d++ )
		{
			auto& Device = mDevices[d];
			Devices[DeviceCount++] = Device.mDeviceId;
		}
		return DeviceCount;
	}

};
