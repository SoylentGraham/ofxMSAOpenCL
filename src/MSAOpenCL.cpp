#include "MSAOpenCL.h"
#include "MSAOpenCLProgram.h"
#include "MSAOpenCLKernel.h"
//#define ENABLE_SETUP_FROM_OPENGL

namespace msa {
	
	OpenCL *OpenCL::currentOpenCL = NULL;
	
	
	OpenCL::OpenCL() :
		isSetup		( false ),
		clContext	( NULL )
	{
		//ofSetLogLevel( OF_LOG_VERBOSE );
		ofLog(OF_LOG_VERBOSE, __FUNCTION__ );
	}
	
	OpenCL::~OpenCL() {
		ofLog(OF_LOG_VERBOSE, __FUNCTION__ );
		
		for ( int q=0;	q<mQueues.size();	q++ )
			clFinish( mQueues[q] );
		
		ofMutex::ScopedLock MemObjectsLock(mMemObjectsLock);
		for(int i=0; i<memObjects.size(); i++) 
			delete memObjects[i];

		ofMutex::ScopedLock KernelsLock(mKernelsLock);
		for(int i=0;	i<kernels.size();	i++ )
			delete kernels[i];

		ofMutex::ScopedLock ProgramsLock(mProgramsLock);
		for(int i=0; i<programs.size(); i++) 
			delete programs[i];

		for ( int q=0;	q<mQueues.size();	q++ )
			clReleaseCommandQueue( mQueues[q] );

		clReleaseContext(clContext);
	}
	
	
	bool OpenCL::setup(int clDeviceType) {
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ + " " + ofToString(clDeviceType));
		
		if(isSetup) {
			ofLog(OF_LOG_VERBOSE, "... already setup. returning");
			return true;
		}
		
		if ( !createDevice(clDeviceType) )
			return false;
		
		cl_device_id DeviceBuffer[100];
		int DeviceCount = 0;
		for ( int d=0;	d<min(sizeof(DeviceBuffer)/sizeof(DeviceBuffer[0]),mDevices.size());	d++ )
			DeviceBuffer[DeviceCount++] = mDevices[d];

		cl_int err;
		clContext = clCreateContext(NULL, DeviceCount, DeviceBuffer, NULL, NULL, &err);
		if( !clContext || err != CL_SUCCESS )
		{
			ofLog(OF_LOG_ERROR, "Error creating clContext.");
			assert(err != CL_INVALID_PLATFORM);
			assert(err != CL_INVALID_VALUE);
			assert(err != CL_INVALID_DEVICE);
			assert(err != CL_INVALID_DEVICE_TYPE);
			assert(err != CL_DEVICE_NOT_AVAILABLE);
			assert(err != CL_DEVICE_NOT_FOUND);
			assert(err != CL_OUT_OF_HOST_MEMORY);
			assert(false);
			return false;
		}
		
		
		createQueue();
		return true;
	}	
	
	
	bool OpenCL::setupFromOpenGL() {
#if defined(ENABLE_SETUP_FROM_OPENGL)
		ofLog(OF_LOG_VERBOSE, "OpenCL::setupFromOpenGL ");
		
		if(isSetup) {
			ofLog(OF_LOG_VERBOSE, "... already setup. returning");
			return true;
		}
		
		cl_int err;
		
		createDevice(CL_DEVICE_TYPE_GPU, 1);
		
#ifdef TARGET_OSX	
		CGLContextObj kCGLContext = CGLGetCurrentContext();
		CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
		cl_context_properties properties[] = { CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup, 0 };
#else
		ofLog(OF_LOG_ERROR, "OpenCL::setupFromOpenGL() only implemented for mac osx at the moment.\nIf you know how to do this for windows/linux please go ahead and implement it here.");
		assert(false);
#endif
		
		clContext = clCreateContext(properties, 0, 0, NULL, NULL, &err);
		if(clContext == NULL) {
			ofLog(OF_LOG_ERROR, "Error creating clContext.");
			assert(err != CL_INVALID_PLATFORM);
			assert(err != CL_INVALID_VALUE);
			assert(err != CL_INVALID_DEVICE);
			assert(err != CL_INVALID_DEVICE_TYPE);
			assert(err != CL_DEVICE_NOT_AVAILABLE);
			assert(err != CL_DEVICE_NOT_FOUND);
			assert(err != CL_OUT_OF_HOST_MEMORY);
			assert(false);
		}
		
		createQueue();
#endif
		return false;
	}	
	
	
	cl_device_id& OpenCL::getDevice() {
		return mDevices[0];
	}
	
	
	cl_context& OpenCL::getContext() {
		return clContext;
	}
	
	cl_command_queue& OpenCL::getQueue() {
		return mQueues[0];
	}
	
	
	
	OpenCLProgram* OpenCL::loadProgramFromFile(string filename, bool isBinary,const char* BuildOptions) { 
		assert( isInitialised() );
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ + " " + filename );
		OpenCLProgram *p = new OpenCLProgram();
		p->loadFromFile(filename, isBinary, BuildOptions );

		ofMutex::ScopedLock Lock(mProgramsLock);
		programs.push_back(p);
		return p;
	}
	
	
	OpenCLProgram* OpenCL::loadProgramFromSource(string source) {
		assert( isInitialised() );
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		OpenCLProgram *p = new OpenCLProgram();
		p->loadFromSource(source,NULL,NULL);

		ofMutex::ScopedLock Lock(mProgramsLock);
		programs.push_back(p);
		return p;
	} 
	
	
	OpenCLKernel* OpenCL::loadKernel(string kernelName,OpenCLProgram& program,cl_command_queue Queue) {
		assert( isInitialised() );
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ + " " + kernelName + ", " + program.getName() );
		OpenCLKernel *k = program.loadKernel( kernelName, Queue );
		
		ofMutex::ScopedLock Lock(mKernelsLock);
		kernels.push_back(k);
		return k;
	}
	
	void OpenCL::deleteKernel(OpenCLKernel& Kernel)
	{
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		assert( isInitialised() );
		delete &Kernel;
		ofMutex::ScopedLock Lock(mKernelsLock);
		auto it = std::find( kernels.begin(), kernels.end(), &Kernel );
		kernels.erase( it );
	}

	void OpenCL::deleteBuffer(OpenCLMemoryObject& Object)
	{
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		assert( isInitialised() );
		delete &Object;
		ofMutex::ScopedLock Lock(mMemObjectsLock);
		auto it = std::find( memObjects.begin(), memObjects.end(), &Object );
		memObjects.erase( it );
	}

	OpenCLBuffer* OpenCL::createBuffer(int numberOfBytes, cl_mem_flags memFlags, void *dataPtr, bool blockingWrite,cl_command_queue Queue) {
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ + " " + ofToString(numberOfBytes,0) + " bytes; blocking: " + ofToString(blockingWrite,0)  );
		assert( isInitialised() );
		OpenCLBuffer *clBuffer = new OpenCLBuffer();
		if (!clBuffer )
			return NULL;
		clBuffer->initBuffer(numberOfBytes, memFlags, dataPtr, blockingWrite,Queue);

		ofMutex::ScopedLock Lock(mMemObjectsLock);
		memObjects.push_back(clBuffer);
		ofLog(OF_LOG_VERBOSE, string("FINISHED: ") + __FUNCTION__ + " " + ofToString(numberOfBytes,0) + " bytes; blocking: " + ofToString(blockingWrite,0)  );
		return clBuffer;
	}
	
	
	OpenCLBuffer* OpenCL::createBufferFromGLObject(GLuint glBufferObject, cl_mem_flags memFlags) {
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		assert( isInitialised() );
		OpenCLBuffer *clBuffer = new OpenCLBuffer();
		clBuffer->initFromGLObject(glBufferObject, memFlags);

		ofMutex::ScopedLock Lock(mMemObjectsLock);
		memObjects.push_back(clBuffer);
		return clBuffer;
	}
	
	
	OpenCLImage* OpenCL::createImage2D(int width, int height, cl_channel_order imageChannelOrder, cl_channel_type imageChannelDataType, cl_mem_flags memFlags, void *dataPtr, bool blockingWrite) {
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		assert( isInitialised() );
		return createImage3D(width, height, 1, imageChannelOrder, imageChannelDataType, memFlags, dataPtr, blockingWrite);
	}
	
	
	OpenCLImage* OpenCL::createImageFromTexture(ofTexture &tex, cl_mem_flags memFlags, int mipLevel) {
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		assert( isInitialised() );
		OpenCLImage *clImage = new OpenCLImage();
		clImage->initFromTexture(tex, memFlags, mipLevel);

		ofMutex::ScopedLock Lock(mMemObjectsLock);
		memObjects.push_back(clImage);
		return clImage;
	}
	
	OpenCLImage* OpenCL::createImageWithTexture(int width, int height, int glType, cl_mem_flags memFlags) {
		assert( isInitialised() );
		OpenCLImage *clImage = new OpenCLImage();
		clImage->initWithTexture(width, height, glType, memFlags);
		
		ofMutex::ScopedLock Lock(mMemObjectsLock);
		memObjects.push_back(clImage);
		return clImage;
	}
	
	
	OpenCLImage* OpenCL::createImage3D(int width, int height, int depth, cl_channel_order imageChannelOrder, cl_channel_type imageChannelDataType, cl_mem_flags memFlags, void *dataPtr, bool blockingWrite) {
		assert( isInitialised() );
		OpenCLImage *clImage = new OpenCLImage();
		clImage->initWithoutTexture(width, height, depth, imageChannelOrder, imageChannelDataType, memFlags, dataPtr, blockingWrite);
	
		ofMutex::ScopedLock Lock(mMemObjectsLock);
		memObjects.push_back(clImage);
		return clImage;
	}
	
	
	void OpenCL::flush() {
		assert( isInitialised() );
		clFlush( getQueue() );
	}
	
	
	void OpenCL::finish() {
		assert( isInitialised() );
		clFinish( getQueue() );
	}
	
	
	
	bool OpenCL::createDevice(int clDeviceType) 
	{
		cl_int err;
		
		cl_platform_id PlatformBuffer[100];
		cl_uint PlatformCount = 0;
		const int MaxPlatforms = sizeof(PlatformBuffer)/sizeof(PlatformBuffer[0]);

		//	windows AMD sdk/ati radeon driver implementation doesn't accept NULL as a platform ID, so fetch it first
		err = clGetPlatformIDs(	MaxPlatforms, PlatformBuffer, &PlatformCount );
		assert( PlatformCount <= MaxPlatforms );

		//	error fetching platforms... try NULL anyway
		if ( err != CL_SUCCESS || PlatformCount == 0 )
		{
			PlatformBuffer[0] = NULL;
			PlatformCount = 1;
		}

		//	collect devices on each platform
		for ( int p=0;	p<PlatformCount;	p++ )
		{
			cl_platform_id Platform = PlatformBuffer[p];
			cl_device_id DeviceBuffer[100];
			cl_uint DeviceCount = 0;
			const int MaxDevices = sizeof(DeviceBuffer)/sizeof(DeviceBuffer[0]);

			err = clGetDeviceIDs( Platform, clDeviceType, MaxDevices, DeviceBuffer, &DeviceCount);
			assert( DeviceCount <= MaxDevices );
			if ( err != CL_SUCCESS )
				continue;

			//	save devices
			for ( int d=0;	d<DeviceCount;	d++ )
				mDevices.push_back( DeviceBuffer[d] );			
		}

		ofLog(OF_LOG_VERBOSE, ofToString(mDevices.size(), 0) + " devices found, on " + ofToString(PlatformCount, 0) + " platforms\n");
		if ( mDevices.size() == 0 )
		{
			assert(false);
			return false;
		}
		
		for(int i=0; i<mDevices.size(); i++) {
			size_t	size;
			cl_device_id d = mDevices[i];
			err = clGetDeviceInfo(d, CL_DEVICE_VENDOR, sizeof(info.vendorName), info.vendorName, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_NAME, sizeof(info.deviceName), info.deviceName, &size);
			err |= clGetDeviceInfo(d, CL_DRIVER_VERSION, sizeof(info.driverVersion), info.driverVersion, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_VERSION, sizeof(info.deviceVersion), info.deviceVersion, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(info.maxComputeUnits), &info.maxComputeUnits, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(info.maxWorkItemDimensions), &info.maxWorkItemDimensions, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(info.maxWorkItemSizes), &info.maxWorkItemSizes, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(info.maxWorkGroupSize), &info.maxWorkGroupSize, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(info.maxClockFrequency), &info.maxClockFrequency, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(info.maxMemAllocSize), &info.maxMemAllocSize, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE_SUPPORT, sizeof(info.imageSupport), &info.imageSupport, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(info.maxReadImageArgs), &info.maxReadImageArgs, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(info.maxWriteImageArgs), &info.maxWriteImageArgs, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(info.image2dMaxWidth), &info.image2dMaxWidth, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(info.image2dMaxHeight), &info.image2dMaxHeight, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE3D_MAX_WIDTH, sizeof(info.image3dMaxWidth), &info.image3dMaxWidth, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE3D_MAX_HEIGHT, sizeof(info.image3dMaxHeight), &info.image3dMaxHeight, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_IMAGE3D_MAX_DEPTH, sizeof(info.image3dMaxDepth), &info.image3dMaxDepth, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_SAMPLERS, sizeof(info.maxSamplers), &info.maxSamplers, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_PARAMETER_SIZE, sizeof(info.maxParameterSize), &info.maxParameterSize, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(info.globalMemCacheSize), &info.globalMemCacheSize, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(info.globalMemSize), &info.globalMemSize, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(info.maxConstantBufferSize), &info.maxConstantBufferSize, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_MAX_CONSTANT_ARGS, sizeof(info.maxConstantArgs), &info.maxConstantArgs, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(info.localMemSize), &info.localMemSize, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_ERROR_CORRECTION_SUPPORT, sizeof(info.errorCorrectionSupport), &info.errorCorrectionSupport, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof(info.profilingTimerResolution), &info.profilingTimerResolution, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_ENDIAN_LITTLE, sizeof(info.endianLittle), &info.endianLittle, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_PROFILE, sizeof(info.profile), info.profile, &size);
			err |= clGetDeviceInfo(d, CL_DEVICE_EXTENSIONS, sizeof(info.extensions), info.extensions, &size);
			
			if(err != CL_SUCCESS) {
				ofLog(OF_LOG_ERROR, "Error getting clDevice information.");
				assert(false);
			}
			
			ofLog(OF_LOG_VERBOSE, getInfoAsString());
		}
				
		return true;
	}
	
	string OpenCL::getInfoAsString() {
		return string("\n\n*********\nOpenCL Device information:") + 
		"\n vendorName.................." + string((char*)info.vendorName) + 
		"\n deviceName.................." + string((char*)info.deviceName) + 
		"\n driverVersion..............." + string((char*)info.driverVersion) +
		"\n deviceVersion..............." + string((char*)info.deviceVersion) +
		"\n maxComputeUnits............." + ofToString(info.maxComputeUnits, 0) +
		"\n maxWorkItemDimensions......." + ofToString(info.maxWorkItemDimensions, 0) +
		"\n maxWorkItemSizes[0]........." + ofToString(info.maxWorkItemSizes[0], 0) + 
		"\n maxWorkGroupSize............" + ofToString(info.maxWorkGroupSize, 0) +
		"\n maxClockFrequency..........." + ofToString(info.maxClockFrequency, 0) +
		"\n maxMemAllocSize............." + ofToString(info.maxMemAllocSize/1024.0f/1024.0f, 3) + " MB" + 
		"\n imageSupport................" + (info.imageSupport ? "YES" : "NO") +
		"\n maxReadImageArgs............" + ofToString(info.maxReadImageArgs, 0) +
		"\n maxWriteImageArgs..........." + ofToString(info.maxWriteImageArgs, 0) +
		"\n image2dMaxWidth............." + ofToString(info.image2dMaxWidth, 0) +
		"\n image2dMaxHeight............" + ofToString(info.image2dMaxHeight, 0) +
		"\n image3dMaxWidth............." + ofToString(info.image3dMaxWidth, 0) +
		"\n image3dMaxHeight............" + ofToString(info.image3dMaxHeight, 0) +
		"\n image3dMaxDepth............." + ofToString(info.image3dMaxDepth, 0) +
		"\n maxSamplers................." + ofToString(info.maxSamplers, 0) +
		"\n maxParameterSize............" + ofToString(info.maxParameterSize, 0) +
		"\n globalMemCacheSize.........." + ofToString(info.globalMemCacheSize/1024.0f/1024.0f, 3) + " MB" + 
		"\n globalMemSize..............." + ofToString(info.globalMemSize/1024.0f/1024.0f, 3) + " MB" +
		"\n maxConstantBufferSize......." + ofToString(info.maxConstantBufferSize/1024.0f, 3) + " KB"
		"\n maxConstantArgs............." + ofToString(info.maxConstantArgs, 0) +
		"\n localMemSize................" + ofToString(info.localMemSize/1024.0f, 3) + " KB"
		"\n errorCorrectionSupport......" + (info.errorCorrectionSupport ? "YES" : "NO") +
		"\n profilingTimerResolution...." + ofToString(info.profilingTimerResolution, 0) +
		"\n endianLittle................" + ofToString(info.endianLittle, 0) +
		"\n profile....................." + string((char*)info.profile) +
		"\n extensions.................." + string((char*)info.extensions) +
		"\n*********\n\n";
	}
	
	const char* OpenCL::getErrorAsString(cl_int err)
	{
		switch ( err )
		{
			case CL_SUCCESS:					return "Success";
			case CL_DEVICE_NOT_FOUND:			return "Device not found";
			case CL_DEVICE_NOT_AVAILABLE:		return "Device not available";
			case CL_COMPILER_NOT_AVAILABLE:		return "Compiler not available";
			case CL_MEM_OBJECT_ALLOCATION_FAILURE:	return "Memory object allocation failure";
			case CL_OUT_OF_RESOURCES:			return "Out of resources";
			case CL_OUT_OF_HOST_MEMORY:			return "Out of host memory";
			case CL_PROFILING_INFO_NOT_AVAILABLE:	return "Profiling info not available";
			case CL_MEM_COPY_OVERLAP:			return "Memory copy overlap";
			case CL_IMAGE_FORMAT_MISMATCH:		return "Image format mismatch";
			case CL_IMAGE_FORMAT_NOT_SUPPORTED:	return "Image format not supported";
			case CL_BUILD_PROGRAM_FAILURE:		return "Build program failure";
			case CL_MAP_FAILURE:				return "Map failure";
			case CL_INVALID_VALUE:				return "Invalid value";
			case CL_INVALID_DEVICE_TYPE:		return "Invalid device type";
			case CL_INVALID_PLATFORM:			return "Invalid platform";
			case CL_INVALID_DEVICE:				return "Invalid device";
			case CL_INVALID_CONTEXT:			return "Invalid context";
			case CL_INVALID_QUEUE_PROPERTIES:	return "Invalid queue properties";
			case CL_INVALID_COMMAND_QUEUE:		return "Invalid command queue";
			case CL_INVALID_HOST_PTR:			return "Invalid host pointer";
			case CL_INVALID_MEM_OBJECT:			return "Invalid memory object";
			case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:	return "Invalid image format descriptor";
			case CL_INVALID_IMAGE_SIZE:			return "Invalid image size";
			case CL_INVALID_SAMPLER:			return "Invalid sampler";
			case CL_INVALID_BINARY:				return "Invalid binary";
			case CL_INVALID_BUILD_OPTIONS:		return "Invalid build options";
			case CL_INVALID_PROGRAM:			return "Invalid program";
			case CL_INVALID_PROGRAM_EXECUTABLE:	return "Invalid program executable";
			case CL_INVALID_KERNEL_NAME:		return "Invalid kernel name";
			case CL_INVALID_KERNEL_DEFINITION:	return "Invalid kernel definition";
			case CL_INVALID_KERNEL:				return "Invalid kernel";
			case CL_INVALID_ARG_INDEX:			return "Invalid argument index";
			case CL_INVALID_ARG_VALUE:			return "Invalid argument value";
			case CL_INVALID_ARG_SIZE:			return "Invalid argument size";
			case CL_INVALID_KERNEL_ARGS:		return "Invalid kernel arguments";
			case CL_INVALID_WORK_DIMENSION:		return "Invalid work dimension";
			case CL_INVALID_WORK_GROUP_SIZE:	return "Invalid work group size";
			case CL_INVALID_WORK_ITEM_SIZE:		return "invalid work item size";
			case CL_INVALID_GLOBAL_OFFSET:		return "Invalid global offset";
			case CL_INVALID_EVENT_WAIT_LIST:	return "Invalid event wait list";
			case CL_INVALID_EVENT:				return "Invalid event";
			case CL_INVALID_OPERATION:			return "Invalid operation";
			case CL_INVALID_GL_OBJECT:			return "Invalid OpenGL object";
			case CL_INVALID_BUFFER_SIZE:		return "Invalid buffer size";
			case CL_INVALID_MIP_LEVEL:			return "Invalid MIP level";
		}
    
		//	unhandled case
		assert(false);
		return "Unhandled opencl error";
	}

	
	void OpenCL::createQueue() {
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		if ( !mDevices.size() )
			return;

		cl_command_queue Queue = clCreateCommandQueue(clContext, getDevice(), 0, NULL);
		if( !Queue )
		{
			ofLog(OF_LOG_ERROR, "Error creating command queue.");
			assert(false);
			return;
		}
		mQueues.push_back( Queue );
		
		isSetup = true;
		currentOpenCL = this;
	}
	
	cl_command_queue OpenCL::createNewQueue() {
		ofLog(OF_LOG_VERBOSE, string() + __FUNCTION__ );
		assert( isInitialised() );
		cl_command_queue Queue = clCreateCommandQueue(clContext, getDevice(), 0, NULL);
		return Queue;
	}
	
}
