#pragma once

#include "ofMain.h"

namespace msa { 
	
	class OpenCL;
	class OpenCLKernel;
	
	class OpenCLProgram {
	public:
		OpenCLProgram();
		~OpenCLProgram();
		
		bool loadFromFile(string filename,bool isBinary,const char* BuildOptions);
		bool loadFromSource(string source,const char* sourceLocation,const char* BuildOptions);
		
		OpenCLKernel*	loadKernel(string kernelName);
		
		void			getBinary();
		const string&	getName() const	{	return mName;	}
		
		cl_program&		getCLProgram();
		
	protected:	
		bool			build(const char* sourceLocation,const char* BuildOptions);

	protected:	
		OpenCL*			pOpenCL;
		cl_program		clProgram;
		string			mName;		//	for debug purposes only, most likely the filename the program was loaded from
	};
	
}