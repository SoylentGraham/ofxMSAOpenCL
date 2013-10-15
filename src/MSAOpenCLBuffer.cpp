#include "MSAOpenCL.h"
#include "MSAOpenCLBuffer.h"

namespace msa {
	
	OpenCLBuffer::OpenCLBuffer() {
		ofLog(OF_LOG_VERBOSE, "OpenCLBuffer::OpenCLBuffer");
	}
	
	bool OpenCLBuffer::initBuffer(int numberOfBytes,
								  cl_mem_flags memFlags,
								  void *dataPtr,
								  bool blockingWrite,cl_command_queue Queue)
	{
		
		ofLog(OF_LOG_VERBOSE, "OpenCLBuffer::initBuffer");
		
		init();
		
		//	buffer of zero will fail
		if ( numberOfBytes <= 0 )
			return false;

		//	trying to allocate something we KNOW is too big
		if ( numberOfBytes > pOpenCL->info.maxMemAllocSize )
			return false;

		//	do not lose existing pointers
		assert( !clMemObject );
		cl_int err;
		clMemObject = clCreateBuffer(pOpenCL->getContext(), memFlags, numberOfBytes, memFlags & CL_MEM_USE_HOST_PTR ? dataPtr : NULL, &err);
		assert(err == CL_SUCCESS);
		if ( err != CL_SUCCESS )
			return false;
		assert(clMemObject);
		if ( !clMemObject )
			return false;
		
		if(dataPtr) 
			if ( !write( dataPtr, 0, numberOfBytes, blockingWrite, Queue ) )
				return false;

		return true;
	}
	
	
	void OpenCLBuffer::initFromGLObject(GLuint glBufferObject,
										cl_mem_flags memFlags)
	{	
		ofLog(OF_LOG_VERBOSE, "OpenCLBuffer::initFromGLObject");
		
		init();
		
		cl_int err;
		clMemObject= clCreateFromGLBuffer(pOpenCL->getContext(), memFlags, glBufferObject, &err);
		assert(err != CL_INVALID_CONTEXT);
		assert(err != CL_INVALID_VALUE);
		assert(err != CL_INVALID_GL_OBJECT);
		assert(err != CL_OUT_OF_HOST_MEMORY);
		assert(err == CL_SUCCESS);
		assert(clMemObject);	
	}
	
	
	bool OpenCLBuffer::read(void *dataPtr, int startOffsetBytes, int numberOfBytes, bool blockingRead,cl_command_queue Queue) {
		if ( !Queue )
			Queue = OpenCL::currentOpenCL->getQueue();
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
		ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
#endif
		cl_int err = clEnqueueReadBuffer( Queue, clMemObject, blockingRead, startOffsetBytes, numberOfBytes, dataPtr, 0, NULL, NULL);
		assert(err == CL_SUCCESS);
		return err == CL_SUCCESS;
	}
	
	
	bool OpenCLBuffer::write(void *dataPtr, int startOffsetBytes, int numberOfBytes, bool blockingWrite,cl_command_queue Queue) {
		if ( !Queue )
			Queue = OpenCL::currentOpenCL->getQueue();
#if defined(ENABLE_OPENCL_RELEASE_LOCK)
		ofMutex::ScopedLock Lock(OpenCLMemoryObject::gReleaseLock);
#endif
		cl_int err = clEnqueueWriteBuffer( Queue, clMemObject, blockingWrite, startOffsetBytes, numberOfBytes, dataPtr, 0, NULL, NULL);
		assert(err == CL_SUCCESS);
		return err == CL_SUCCESS;
	}
	
	bool OpenCLBuffer::copyFrom(OpenCLBuffer &srcBuffer, int srcOffsetBytes, int dstOffsetBytes, int numberOfBytes,cl_command_queue Queue) {
		if ( !Queue )
			Queue = OpenCL::currentOpenCL->getQueue();
		cl_int err = clEnqueueCopyBuffer( Queue, srcBuffer.getCLMem(), clMemObject, srcOffsetBytes, dstOffsetBytes, numberOfBytes, 0, NULL, NULL);
		assert(err == CL_SUCCESS);
		return err == CL_SUCCESS;
	}
	
	
	void OpenCLBuffer::init() {
		memoryObjectInit();
	}
}
