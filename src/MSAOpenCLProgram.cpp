#include "MSAOpenCL.h"
#include "MSAOpenCLProgram.h"
#include "MSAOpenCLKernel.h"

namespace msa { 
	
	char *OpenCL_textFileRead(char *fn);
	
	
	OpenCLProgram::OpenCLProgram() {
		ofLog(OF_LOG_VERBOSE, "OpenCLProgram::OpenCLProgram");
		this->pOpenCL = pOpenCL;
		pOpenCL = NULL;
		clProgram = NULL;
	}
	
	
	OpenCLProgram::~OpenCLProgram() {
		ofLog(OF_LOG_VERBOSE, "OpenCLProgram::~OpenCLProgram");
		//	clReleaseProgram(clProgram);		// this crashes it for some reason
	}
	
	
	bool OpenCLProgram::loadFromFile(std::string filename, bool isBinary,const char* BuildOptions) { 
		ofLog(OF_LOG_VERBOSE, "OpenCLProgram::loadFromFile " + filename + ", isBinary: " + ofToString(isBinary) + ", buildoptions: " + BuildOptions );
		
		string fullPath = ofToDataPath(filename.c_str());
		
		if(isBinary) {
			//		clCreateProgramWithBinary
			ofLog(OF_LOG_ERROR, "Binary programs not implemented yet\n");
			return false;
			
		} else {
			
			char *source = OpenCL_textFileRead( const_cast<char*>(fullPath.c_str()) );
			if(source == NULL) {
				ofLog(OF_LOG_ERROR, "Error loading program file: " + fullPath);
				return false;
			}
			
			bool Success = loadFromSource( source, filename.c_str(), BuildOptions );
			
			free(source);

			return Success;
		}
	}
	
	
	
	bool OpenCLProgram::loadFromSource(std::string source,const char* sourceLocation,const char* BuildOptions) {
		ofLog(OF_LOG_VERBOSE, "OpenCLProgram::loadFromSource ");// + source);
		
		cl_int err;
		
		pOpenCL = OpenCL::currentOpenCL;
		
		const char* csource = source.c_str();
		clProgram = clCreateProgramWithSource(pOpenCL->getContext(), 1, &csource, NULL, &err);
		
		return build( sourceLocation ? sourceLocation : "(From source)", BuildOptions );
	} 
	
	
	OpenCLKernel* OpenCLProgram::loadKernel(string kernelName) {
		ofLog(OF_LOG_VERBOSE, "OpenCLProgram::loadKernel " + kernelName);
		if ( !clProgram )
			return NULL;
		
		cl_int err;
		
		OpenCLKernel *k = new OpenCLKernel(pOpenCL, clCreateKernel(clProgram, kernelName.c_str(), &err), kernelName);
		
		if(err != CL_SUCCESS) {
			ofLog(OF_LOG_ERROR, string("Error creating kernel: ") + kernelName + " [" + OpenCL::getErrorAsString(err) + "]" );
			delete k;
			return NULL;
		}
		
		return k;
	}
	
	
	void OpenCLProgram::getBinary()
	{
		cl_uint program_num_devices;
		cl_int err;
		err = clGetProgramInfo(clProgram, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint), &program_num_devices, NULL);
		assert(err == CL_SUCCESS);
		
		if (program_num_devices == 0) {
			std::cerr << "no valid binary was found" << std::endl;
			return;
		}
		
		size_t* binaries_sizes = new size_t[program_num_devices];
		
		err = clGetProgramInfo(clProgram, CL_PROGRAM_BINARY_SIZES, program_num_devices*sizeof(size_t), binaries_sizes, NULL);
		assert(err = CL_SUCCESS);
		
		char **binaries = new char*[program_num_devices];
		
		for (size_t i = 0; i < program_num_devices; i++)
			binaries[i] = new char[binaries_sizes[i]+1];
		
		err = clGetProgramInfo(clProgram, CL_PROGRAM_BINARIES, program_num_devices*sizeof(size_t), binaries, NULL);
		assert(err = CL_SUCCESS);
		
		for (size_t i = 0; i < program_num_devices; i++) {
			binaries[i][binaries_sizes[i]] = '\0';
			std::cout << "Program " << i << ":" << std::endl;
			std::cout << binaries[i];
		}
		
		for (size_t i = 0; i < program_num_devices; i++)
			delete [] binaries[i];
		
		delete [] binaries;
		delete [] binaries_sizes;
	}
	
	
	bool OpenCLProgram::build(const char* ProgramSource,const char* BuildOptions) {
		if(clProgram == NULL) {
			ofLog(OF_LOG_ERROR, "Error creating program object.");
			return false;
		}	
		
		string Options;
		string Path = ofToDataPath("",true).c_str();

		//	opencl (amd) requires paths with forward slashes
		std::replace( Path.begin(), Path.end(), '\\', '/' );
		Options += "-I \"" + Path + "\" ";
		if ( BuildOptions )
			Options += BuildOptions;
	
		string Debug;
		Debug += "Build OpenCLProgram ";
		Debug += ProgramSource;
		Debug += ": (" + Options + ") ";
		ofLog(OF_LOG_NOTICE, Debug + "...");

		cl_int err = clBuildProgram(clProgram, 0, NULL, Options.c_str(), NULL, NULL);
		
		if ( err == CL_SUCCESS )
			Debug += "CL_SUCESS";
		else
			Debug += string("Failed [") + OpenCL::getErrorAsString(err) + "]";
		Debug += "\n------------------------------------\n\n";
		ofLog( (err == CL_SUCCESS) ? OF_LOG_NOTICE : OF_LOG_ERROR, Debug);

		//	get build log size first so we always have errors to display
		size_t len = 0;
		int BuildInfoErr = clGetProgramBuildInfo(clProgram, pOpenCL->getDevice(), CL_PROGRAM_BUILD_LOG, 0, NULL, &len );
		vector<char> buffer( len+1 );
		BuildInfoErr = clGetProgramBuildInfo(clProgram, pOpenCL->getDevice(), CL_PROGRAM_BUILD_LOG, buffer.size(), &buffer.at(0), NULL );
		buffer[len] = '\0';
			
		if ( len > 1 )
		{
			//	errors might contain % symbols, which will screw up the va_args system when it tries to parse them...
			std::replace( buffer.begin(), buffer.end(), '%', '@' );

			const char* bufferString = &buffer[0];
			ofLog(OF_LOG_ERROR, bufferString );
			ofLog( (err == CL_SUCCESS) ? OF_LOG_NOTICE : OF_LOG_ERROR, "\n------------------------------------\n");
		}
		return ( err == CL_SUCCESS );
	}
	
	cl_program& OpenCLProgram::getCLProgram(){
		return clProgram;	
	}
	
	//---------------------------------------------------------
	// below is from: www.lighthouse3d.com
	// you may use these functions freely. they are provided as is, and no warranties, either implicit, or explicit are given
	//---------------------------------------------------------
	
	char *OpenCL_textFileRead(char *fn) {
		
		FILE *fp;
		char *content 	= 	NULL;
		int count		=	0;
		
		if (fn != NULL) {
			fp = fopen(fn,"rt");
			if (fp != NULL) {
				
				fseek(fp, 0, SEEK_END);
				count = ftell(fp);
				rewind(fp);
				
				if (count > 0) {
					content = (char *)malloc(sizeof(char) * (count+1));
					count = fread(content,sizeof(char),count,fp);
					content[count] = '\0';
				}
				fclose(fp);
			}
		}
		
		return content;
	}
}
