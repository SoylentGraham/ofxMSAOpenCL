#include "MSAOpenCL.h"
#include "MSAOpenCLMemoryObject.h"

namespace msa { 

	ofMutex OpenCLMemoryObject::gReleaseLock;

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
			ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
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