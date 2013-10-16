/***********************************************************************
 
 OpenCL Buffer Memory Object
 You can either instantiate this class directly. e.g.:
 OpenCLBuffer myBuffer;
 myImage.initFromGLObject(myVbo);
 
 or create it via Instead use OpenCL::createBufferXXXX. e.g.:
 OpenCLBuffer *myBuffer = openCL.createBufferFromGLObject(myVBO);
 (this method is here for backwards compatibility with previous versions of OpenCL)
 
 ************************************************************************/

#pragma once

#include "ofMain.h"
#include "MSAOpenCLMemoryObject.h"

namespace msa {
 	
	class OpenCLBuffer : public OpenCLMemoryObject {
	public:
		
		OpenCLBuffer();

		// if dataPtr parameter is passed in, data is uploaded immediately
		// parameters with default values can be omited
		bool initBuffer(	int numberOfBytes,
						cl_mem_flags memFlags,
						void *dataPtr,
						bool blockingWrite,cl_command_queue Queue=NULL);
		
		
		// create buffer from the GL Object - e.g. VBO (they share memory space on device)
		// parameters with default values can be omited
		void initFromGLObject(	GLuint glBufferObject,
							  cl_mem_flags memFlags = CL_MEM_READ_WRITE);
		
		
		// read from device memory, into main memoy (into dataPtr)
		bool read(void *dataPtr,
				  int startOffsetBytes,
				  int numberOfBytes,
				  bool blockingRead,cl_command_queue Queue=NULL);
		
		// write from main memory (dataPtr), into device memory
		bool write(void *dataPtr,
				   int startOffsetBytes,
				   int numberOfBytes,
				   bool blockingWrite,cl_command_queue Queue=NULL);
		
		bool writeAsync(void *dataPtr,
				   int startOffsetBytes,
				   int numberOfBytes,
				   cl_event* Event,cl_command_queue Queue=NULL);
		
		
		// copy data from another object on device memory
		bool copyFrom(OpenCLBuffer &srcBuffer,
					  int srcOffsetBytes,
					  int dstOffsetBytes,
					  int numberOfBytes,cl_command_queue Queue=NULL);
		
	protected:
		//	int numberOfBytes;		//dont know how big it is if we pass in globject ?
		
		void init();
	};
}
