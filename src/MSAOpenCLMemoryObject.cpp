#include "MSAOpenCL.h"
#include "MSAOpenCLMemoryObject.h"

namespace msa { 

#if defined(ENABLE_OPENCL_RELEASE_LOCK)
	ofMutex OpenCLMemoryObject::gReleaseLock;
#endif

	OpenCLMemoryObject::OpenCLMemoryObject() :
		pOpenCL		( OpenCL::currentOpenCL ),
		clMemObject	( NULL )
	{
		ofLog(OF_LOG_VERBOSE, "OpenCLMemoryObject::OpenCLMemoryObject");
	}
	
	OpenCLMemoryObject::~OpenCLMemoryObject() {
		ofLog(OF_LOG_VERBOSE, "OpenCLMemoryObject::~OpenCLMemoryObject");
		if(clMemObject) 
		{
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
			ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
#endif
			clReleaseMemObject(clMemObject);
		}
	}
	
	
	cl_mem &OpenCLMemoryObject::getCLMem() {
		return clMemObject;
	}
	
	void OpenCLMemoryObject::memoryObjectInit() {
		ofLog(OF_LOG_VERBOSE, "OpenCLMemoryObject::memoryObjectInit");
	}
}