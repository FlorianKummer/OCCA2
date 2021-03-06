#if OCCA_OPENCL_ENABLED

#include "occaOpenCL.hpp"

namespace occa {
  //---[ Helper Functions ]-----------
  namespace cl {
    cl_device_type deviceType(int type){
      cl_device_type ret = 0;

      if(type & occa::CPU)     ret |= CL_DEVICE_TYPE_CPU;
      if(type & occa::GPU)     ret |= CL_DEVICE_TYPE_GPU;
      if(type & occa::FPGA)    ret |= CL_DEVICE_TYPE_ACCELERATOR;
      if(type & occa::XeonPhi) ret |= CL_DEVICE_TYPE_ACCELERATOR;

      return ret;
    }

    int platformCount(){
      cl_uint platformCount;

      OCCA_CL_CHECK("OpenCL: Get Platform ID Count",
                    clGetPlatformIDs(0, NULL, &platformCount));

      return platformCount;
    }

    cl_platform_id platformID(int pID){
      cl_platform_id *platforms = new cl_platform_id[pID + 1];

      OCCA_CL_CHECK("OpenCL: Get Platform ID",
                    clGetPlatformIDs(pID + 1, platforms, NULL));

      cl_platform_id ret = platforms[pID];

      delete [] platforms;

      return ret;
    }

    int deviceCount(int type){
      int pCount = cl::platformCount();
      int ret = 0;

      for(int p = 0; p < pCount; ++p)
        ret += deviceCountInPlatform(p, type);

      return ret;
    }

    int deviceCountInPlatform(int pID, int type){
      cl_uint dCount;

      cl_platform_id clPID = platformID(pID);

      OCCA_CL_CHECK("OpenCL: Get Device ID Count",
                    clGetDeviceIDs(clPID,
                                   deviceType(type),
                                   0, NULL, &dCount));

      return dCount;
    }

    cl_device_id deviceID(int pID, int dID, int type){
      cl_device_id *devices = new cl_device_id[dID + 1];

      cl_platform_id clPID = platformID(pID);

      OCCA_CL_CHECK("OpenCL: Get Device ID Count",
                    clGetDeviceIDs(clPID,
                                   deviceType(type),
                                   dID + 1, devices, NULL));

      cl_device_id ret = devices[dID];

      delete [] devices;

      return ret;
    }

    std::string deviceStrInfo(cl_device_id clDID,
                              cl_device_info clInfo){
      size_t bytes;

      OCCA_CL_CHECK("OpenCL: Getting Device String Info",
                    clGetDeviceInfo(clDID,
                                    clInfo,
                                    0, NULL, &bytes));

      char *buffer  = new char[bytes + 1];
      buffer[bytes] = '\0';

      OCCA_CL_CHECK("OpenCL: Getting Device String Info",
                    clGetDeviceInfo(clDID,
                                    clInfo,
                                    bytes, buffer, NULL));

      std::string ret = buffer;

      delete [] buffer;

      int firstNS, lastNS;

      for(int i = 0; i < ret.size(); ++i){
        if((ret[i] != ' ') &&
           (ret[i] != '\t') &&
           (ret[i] != '\n')){
          firstNS = i;
          break;
        }
      }

      for(int i = (ret.size() - 1); i > firstNS; --i){
        if((ret[i] != ' ') &&
           (ret[i] != '\t') &&
           (ret[i] != '\n')){
          lastNS = i;
          break;
        }
      }

      return ret.substr(firstNS, (lastNS - firstNS + 1));
    }

    std::string deviceName(int pID, int dID){
      cl_device_id clDID = deviceID(pID, dID);

      return deviceStrInfo(clDID, CL_DEVICE_NAME);
    }

    int deviceType(int pID, int dID){
      cl_device_id clDID = deviceID(pID, dID);
      int ret = 0;

      cl_device_type clDeviceType;

      OCCA_CL_CHECK("OpenCL: Get Device Type",
                    clGetDeviceInfo(clDID,
                                    CL_DEVICE_TYPE,
                                    sizeof(clDeviceType), &clDeviceType, NULL));

      if(clDeviceType & CL_DEVICE_TYPE_CPU)
        ret |= occa::CPU;
      else if(clDeviceType & CL_DEVICE_TYPE_GPU)
        ret |= occa::GPU;

      return ret;
    }

    int deviceVendor(int pID, int dID){
      cl_device_id clDID = deviceID(pID, dID);
      int ret = 0;

      std::string vendor = deviceStrInfo(clDID, CL_DEVICE_VENDOR);

      if(vendor.find("AMD")                    != std::string::npos ||
         vendor.find("Advanced Micro Devices") != std::string::npos ||
         vendor.find("ATI")                    != std::string::npos)
        ret |= occa::AMD;

      else if(vendor.find("Intel") != std::string::npos)
        ret |= occa::Intel;

      else if(vendor.find("Altera") != std::string::npos)
        ret |= occa::Altera;

      else if(vendor.find("Nvidia") != std::string::npos ||
              vendor.find("NVIDIA") != std::string::npos)
        ret |= occa::NVIDIA;

      return ret;
    }

    int deviceCoreCount(int pID, int dID){
      cl_device_id clDID = deviceID(pID, dID);
      cl_uint ret;

      OCCA_CL_CHECK("OpenCL: Get Device Core Count",
                    clGetDeviceInfo(clDID,
                                    CL_DEVICE_MAX_COMPUTE_UNITS,
                                    sizeof(ret), &ret, NULL));

      return ret;
    }

    occa::deviceInfo deviceInfo(int pID, int dID){
      occa::deviceInfo dInfo;

      dInfo.name  = deviceName(pID, dID);
      dInfo.id    = pID*100 + dID;
      dInfo.info  = deviceType(pID, dID) | deviceVendor(pID, dID) | occa::OpenCL;

      if((dInfo.info & occa::GPU) && (dInfo.info & occa::AMD))
        dInfo.preferredMode = occa::OpenCL;

      return dInfo;
    }
  };
  //==================================


  //---[ Kernel ]---------------------
  template <>
  kernel_t<OpenCL>::kernel_t(){
    data = NULL;
    dev  = NULL;

    functionName = "";

    dims  = 1;
    inner = occa::dim(1,1,1);
    outer = occa::dim(1,1,1);

    preferredDimSize_ = 0;

    startTime = (void*) new cl_event;
    endTime   = (void*) new cl_event;
  }

  template <>
  kernel_t<OpenCL>::kernel_t(const kernel_t<OpenCL> &k){
    data = k.data;
    dev  = k.dev;

    functionName = k.functionName;

    dims  = k.dims;
    inner = k.inner;
    outer = k.outer;

    preferredDimSize_ = k.preferredDimSize_;

    startTime = k.startTime;
    endTime   = k.endTime;
  }

  template <>
  kernel_t<OpenCL>& kernel_t<OpenCL>::operator = (const kernel_t<OpenCL> &k){
    data = k.data;
    dev  = k.dev;

    functionName = k.functionName;

    dims  = k.dims;
    inner = k.inner;
    outer = k.outer;

    preferredDimSize_ = k.preferredDimSize_;

    *((cl_event*) startTime) = *((cl_event*) k.startTime);
    *((cl_event*) endTime)   = *((cl_event*) k.endTime);

    return *this;
  }

  template <>
  kernel_t<OpenCL>::~kernel_t(){}

  template <>
  kernel_t<OpenCL>* kernel_t<OpenCL>::buildFromSource(const std::string &filename,
                                                      const std::string &functionName_,
                                                      const kernelInfo &info_){
    OCCA_EXTRACT_DATA(OpenCL, Kernel);

    functionName = functionName_;

    kernelInfo info = info_;
    info.addDefine("OCCA_USING_GPU"   , 1); // [-] Is it really?
    info.addDefine("OCCA_USING_OPENCL", 1);

    info.addOCCAKeywords(occaOpenCLDefines);

    std::stringstream salt;
    salt << "OpenCL"
         << data_.platform << '-' << data_.device
         << info.salt()
         << dev->dHandle->compiler
         << dev->dHandle->compilerFlags
         << functionName;

    std::string cachedBinary = getCachedName(filename, salt.str());

    struct stat buffer;
    const bool fileExists = (stat(cachedBinary.c_str(), &buffer) == 0);

    if(fileExists){
      std::cout << "Found cached binary of [" << filename << "] in [" << cachedBinary << "]\n";
      return buildFromBinary(cachedBinary, functionName);
    }

    if(!haveFile(cachedBinary)){
      waitForFile(cachedBinary);

      return buildFromBinary(cachedBinary, functionName);
    }

    std::string iCachedBinary = createIntermediateSource(filename,
                                                         cachedBinary,
                                                         info);

    cl_int error;

    int fileHandle = ::open(iCachedBinary.c_str(), O_RDWR);
    if(fileHandle == 0){
      printf("File [ %s ] does not exist.\n", iCachedBinary.c_str());
      releaseFile(cachedBinary);
      throw 1;
    }

    struct stat fileInfo;
    const int status = fstat(fileHandle, &fileInfo);

    if(status != 0){
      printf( "File [ %s ] gave a bad fstat.\n" , iCachedBinary.c_str());
      releaseFile(cachedBinary);
      throw 1;
    }

    const uintptr_t cLength = fileInfo.st_size;

    char *cFunction = new char[cLength + 1];
    cFunction[cLength] = '\0';

    ::read(fileHandle, cFunction, cLength);

    ::close(fileHandle);

    data_.program = clCreateProgramWithSource(data_.context, 1, (const char **) &cFunction, &cLength, &error);

    if(error)
      releaseFile(cachedBinary);

    OCCA_CL_CHECK("Kernel (" + functionName + ") : Constructing Program", error);

    std::string catFlags = info.flags + dev->dHandle->compilerFlags;

    error = clBuildProgram(data_.program,
                           1, &data_.deviceID,
                           catFlags.c_str(),
                           NULL, NULL);

    if(error){
      cl_int error;
      char *log;
      uintptr_t logSize;

      clGetProgramBuildInfo(data_.program, data_.deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);

      if(logSize > 2){
        log = new char[logSize+1];

        error = clGetProgramBuildInfo(data_.program, data_.deviceID, CL_PROGRAM_BUILD_LOG, logSize, log, NULL);
        OCCA_CL_CHECK("Kernel (" + functionName + ") : Building Program", error);
        log[logSize] = '\0';

        printf("Kernel (%s): Build Log\n%s", functionName.c_str(), log);

        delete[] log;
      }

      releaseFile(cachedBinary);
    }

    OCCA_CL_CHECK("Kernel (" + functionName + ") : Building Program", error);

    {
      uintptr_t binarySize;
      char *binary;

      error = clGetProgramInfo(data_.program, CL_PROGRAM_BINARY_SIZES, sizeof(uintptr_t), &binarySize, NULL);

      if(error)
        releaseFile(cachedBinary);

      OCCA_CL_CHECK("saveProgramBinary: Getting Binary Sizes", error);

      binary = new char[binarySize + 1];

      error = clGetProgramInfo(data_.program, CL_PROGRAM_BINARIES, sizeof(char*), &binary, NULL);

      if(error)
        releaseFile(cachedBinary);

      OCCA_CL_CHECK("saveProgramBinary: Getting Binary", error);

      FILE *fp = fopen(cachedBinary.c_str(), "wb");
      fwrite(binary, 1, binarySize, fp);
      fclose(fp);

      delete [] binary;
    }

    data_.kernel = clCreateKernel(data_.program, functionName.c_str(), &error);

    if(error)
      releaseFile(cachedBinary);

    OCCA_CL_CHECK("Kernel (" + functionName + "): Creating Kernel", error);

    std::cout << "OpenCL compiled " << filename << " from [" << iCachedBinary << "]\n";

    releaseFile(cachedBinary);

    delete [] cFunction;

    return this;
  }

  template <>
  kernel_t<OpenCL>* kernel_t<OpenCL>::buildFromBinary(const std::string &filename,
                                                      const std::string &functionName_){
    OCCA_EXTRACT_DATA(OpenCL, Kernel);

    functionName = functionName_;

    cl_int binaryError, error;

    int fileHandle = ::open(filename.c_str(), O_RDWR);
    if(fileHandle == 0){
      printf("File [ %s ] does not exist.\n", filename.c_str());
      throw 1;
    }

    struct stat fileInfo;
    const int status = fstat(fileHandle, &fileInfo);

    if(status != 0){
      printf( "File [ %s ] gave a bad fstat.\n" , filename.c_str());
      throw 1;
    }

    const uintptr_t fileSize = fileInfo.st_size;

    unsigned char *cFile = new unsigned char[fileSize + 1];
    cFile[fileSize] = '\0';

    ::read(fileHandle, cFile, fileSize);

    ::close(fileHandle);

    data_.program = clCreateProgramWithBinary(data_.context,
                                              1, &(data_.deviceID),
                                              &fileSize,
                                              (const unsigned char**) &cFile,
                                              &binaryError, &error);
    OCCA_CL_CHECK("Kernel (" + functionName + ") : Constructing Program", binaryError);
    OCCA_CL_CHECK("Kernel (" + functionName + ") : Constructing Program", error);

    error = clBuildProgram(data_.program,
                           1, &data_.deviceID,
                           dev->dHandle->compilerFlags.c_str(),
                           NULL, NULL);

    if(error){
      cl_int error;
      char *log;
      uintptr_t logSize;

      clGetProgramBuildInfo(data_.program, data_.deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);

      if(logSize > 2){
        log = new char[logSize+1];

        error = clGetProgramBuildInfo(data_.program, data_.deviceID, CL_PROGRAM_BUILD_LOG, logSize, log, NULL);
        OCCA_CL_CHECK("Kernel (" + functionName + ") : Building Program", error);
        log[logSize] = '\0';

        printf("Kernel (%s): Build Log\n%s", functionName.c_str(), log);

        delete[] log;
      }
    }

    OCCA_CL_CHECK("Kernel (" + functionName + ") : Building Program", error);

    data_.kernel = clCreateKernel(data_.program, functionName.c_str(), &error);
    OCCA_CL_CHECK("Kernel (" + functionName + "): Creating Kernel", error);

    delete [] cFile;

    return this;
  }

  template <>
  int kernel_t<OpenCL>::preferredDimSize(){
    if(preferredDimSize_)
      return preferredDimSize_;

    OCCA_EXTRACT_DATA(OpenCL, Kernel);

    uintptr_t pds;

    OCCA_CL_CHECK("Kernel: Getting Preferred Dim Size",
                  clGetKernelWorkGroupInfo(data_.kernel,
                                           data_.deviceID,
                                           CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                           sizeof(uintptr_t), &pds, NULL));

    preferredDimSize_ = pds;

    return preferredDimSize_;
  }

#include "operators/occaOpenCLKernelOperators.cpp"

  template <>
  double kernel_t<OpenCL>::timeTaken(){
    cl_event &startEvent = *((cl_event*) startTime);
    cl_event &endEvent   = *((cl_event*) endTime);

    cl_ulong start, end;

    clGetEventProfilingInfo(startEvent, CL_PROFILING_COMMAND_END,
                            sizeof(cl_ulong), &start,
                            NULL);

    clGetEventProfilingInfo(endEvent, CL_PROFILING_COMMAND_START,
                            sizeof(cl_ulong), &end,
                            NULL);

    return 1.0e-9*(end - start);
  }

  template <>
  void kernel_t<OpenCL>::free(){
  }
  //==================================


  //---[ Memory ]---------------------
  template <>
  memory_t<OpenCL>::memory_t(){
    handle = NULL;
    dev    = NULL;
    size = 0;
  }

  template <>
  memory_t<OpenCL>::memory_t(const memory_t<OpenCL> &m){
    handle = m.handle;
    dev    = m.dev;
    size   = m.size;
  }

  template <>
  memory_t<OpenCL>& memory_t<OpenCL>::operator = (const memory_t<OpenCL> &m){
    handle = m.handle;
    dev    = m.dev;
    size   = m.size;

    return *this;
  }

  template <>
  memory_t<OpenCL>::~memory_t(){}

  template <>
  void memory_t<OpenCL>::copyFrom(const void *source,
                                  const uintptr_t bytes,
                                  const uintptr_t offset){
    cl_command_queue &stream = *((cl_command_queue*) dev->currentStream);

    const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

    OCCA_CHECK((bytes_ + offset) <= size);

    OCCA_CL_CHECK("Memory: Copy From",
                  clEnqueueWriteBuffer(stream, *((cl_mem*) handle),
                                       CL_TRUE,
                                       offset, bytes_, source,
                                       0, NULL, NULL));
  }

  template <>
  void memory_t<OpenCL>::copyFrom(const memory_v *source,
                                  const uintptr_t bytes,
                                  const uintptr_t destOffset,
                                  const uintptr_t srcOffset){
    cl_command_queue &stream = *((cl_command_queue*) dev->currentStream);

    const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

    OCCA_CHECK((bytes_ + destOffset) <= size);
    OCCA_CHECK((bytes_ + srcOffset)  <= source->size);

    OCCA_CL_CHECK("Memory: Copy From",
                  clEnqueueCopyBuffer(stream,
                                      *((cl_mem*) source->handle),
                                      *((cl_mem*) handle),
                                      srcOffset, destOffset,
                                      bytes_,
                                      0, NULL, NULL));
  }

  template <>
  void memory_t<OpenCL>::copyTo(void *dest,
                                const uintptr_t bytes,
                                const uintptr_t offset){
    const cl_command_queue &stream = *((cl_command_queue*) dev->currentStream);

    const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

    OCCA_CHECK((bytes_ + offset) <= size);

    OCCA_CL_CHECK("Memory: Copy To",
                  clEnqueueReadBuffer(stream, *((cl_mem*) handle),
                                      CL_TRUE,
                                      offset, bytes_, dest,
                                      0, NULL, NULL));
  }

  template <>
  void memory_t<OpenCL>::copyTo(memory_v *dest,
                                const uintptr_t bytes,
                                const uintptr_t destOffset,
                                const uintptr_t srcOffset){
    const cl_command_queue &stream = *((cl_command_queue*) dev->currentStream);

    const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

    OCCA_CHECK((bytes_ + srcOffset)  <= size);
    OCCA_CHECK((bytes_ + destOffset) <= dest->size);

    OCCA_CL_CHECK("Memory: Copy To",
                  clEnqueueCopyBuffer(stream,
                                      *((cl_mem*) handle),
                                      *((cl_mem*) dest->handle),
                                      srcOffset, destOffset,
                                      bytes_,
                                      0, NULL, NULL));
  }

  template <>
  void memory_t<OpenCL>::asyncCopyFrom(const void *source,
                                       const uintptr_t bytes,
                                       const uintptr_t offset){
    const cl_command_queue &stream = *((cl_command_queue*) dev->currentStream);

    const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

    OCCA_CHECK((bytes_ + offset) <= size);

    OCCA_CL_CHECK("Memory: Asynchronous Copy From",
                  clEnqueueWriteBuffer(stream, *((cl_mem*) handle),
                                       CL_FALSE,
                                       offset, bytes_, source,
                                       0, NULL, NULL));
  }

  template <>
  void memory_t<OpenCL>::asyncCopyFrom(const memory_v *source,
                                       const uintptr_t bytes,
                                       const uintptr_t destOffset,
                                       const uintptr_t srcOffset){
    const cl_command_queue &stream = *((cl_command_queue*) dev->currentStream);

    const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

    OCCA_CHECK((bytes_ + destOffset) <= size);
    OCCA_CHECK((bytes_ + srcOffset)  <= source->size);

    OCCA_CL_CHECK("Memory: Asynchronous Copy From",
                  clEnqueueCopyBuffer(stream,
                                      *((cl_mem*) source->handle),
                                      *((cl_mem*) handle),
                                      srcOffset, destOffset,
                                      bytes_,
                                      0, NULL, NULL));
  }

  template <>
  void memory_t<OpenCL>::asyncCopyTo(void *dest,
                                     const uintptr_t bytes,
                                     const uintptr_t offset){
    const cl_command_queue &stream = *((cl_command_queue*) dev->currentStream);

    const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

    OCCA_CHECK((bytes_ + offset) <= size);

    OCCA_CL_CHECK("Memory: Asynchronous Copy To",
                  clEnqueueReadBuffer(stream, *((cl_mem*) handle),
                                      CL_FALSE,
                                      offset, bytes_, dest,
                                      0, NULL, NULL));
  }

  template <>
  void memory_t<OpenCL>::asyncCopyTo(memory_v *dest,
                                     const uintptr_t bytes,
                                     const uintptr_t destOffset,
                                     const uintptr_t srcOffset){
    const cl_command_queue &stream = *((cl_command_queue*) dev->currentStream);

    const uintptr_t bytes_ = (bytes == 0) ? size : bytes;

    OCCA_CHECK((bytes_ + srcOffset)  <= size);
    OCCA_CHECK((bytes_ + destOffset) <= dest->size);

    OCCA_CL_CHECK("Memory: Asynchronous Copy To",
                  clEnqueueCopyBuffer(stream,
                                      *((cl_mem*) handle),
                                      *((cl_mem*) dest->handle),
                                      srcOffset, destOffset,
                                      bytes_,
                                      0, NULL, NULL));
  }

  template <>
  void memory_t<OpenCL>::free(){
    clReleaseMemObject(*((cl_mem*) handle));
    delete (cl_mem*) handle;
    size = 0;
  }
  //==================================


  //---[ Device ]---------------------
  template <>
  std::vector<occa::deviceInfo> availableDevices<OpenCL>(){
    std::vector<occa::deviceInfo> ret( occa::cl::deviceCount() );
    int pos = 0;

    int platformCount = occa::cl::platformCount();

    std::cout << occa::deviceInfo::sLine   << '\n'
              << occa::deviceInfo::header << '\n'
              << occa::deviceInfo::sLine   << '\n';

    for(int p = 0; p < platformCount; ++p){
      int deviceCount = occa::cl::deviceCountInPlatform(p);

      for(int d = 0; d < deviceCount; ++d){
        ret[pos] = occa::cl::deviceInfo(p,d);

        std::cout << ret[pos].summarizedInfo() << '\n';

        ++pos;
      }
    }

    std::cout << occa::deviceInfo::sLine << '\n';

    return ret;
  }

  template <>
  device_t<OpenCL>::device_t(){
    data = NULL;
    memoryAllocated = 0;

    getEnvironmentVariables();
  }

  template <>
  device_t<OpenCL>::device_t(int platform, int device){
    data = NULL;
    memoryAllocated = 0;

    getEnvironmentVariables();
  }

  template <>
  device_t<OpenCL>::device_t(const device_t<OpenCL> &d){
    data            = d.data;
    memoryAllocated = d.memoryAllocated;

    compilerFlags = d.compilerFlags;
  }

  template <>
  device_t<OpenCL>& device_t<OpenCL>::operator = (const device_t<OpenCL> &d){
    data            = d.data;
    memoryAllocated = d.memoryAllocated;

    compilerFlags = d.compilerFlags;

    return *this;
  }

  template <>
  void device_t<OpenCL>::setup(const int platform, const int device){
    data = new OpenCLDeviceData_t;

    OCCA_EXTRACT_DATA(OpenCL, Device);
    cl_int error;

    data_.platform = platform;
    data_.device   = device;

    data_.platformID = cl::platformID(platform);
    data_.deviceID   = cl::deviceID(platform, device);

    data_.context = clCreateContext(NULL, 1, &data_.deviceID, NULL, NULL, &error);
    OCCA_CL_CHECK("Device: Creating Context", error);
  }

  template <>
  void device_t<OpenCL>::getEnvironmentVariables(){
    char *c_compilerFlags = getenv("OCCA_OPENCL_COMPILER_FLAGS");
    if(c_compilerFlags != NULL)
      compilerFlags = std::string(c_compilerFlags);
    else{
#if OCCA_DEBUG_ENABLED
      compilerFlags = "-cl-opt-disable";
#else
      compilerFlags = "-cl-single-precision-constant -cl-denorms-are-zero -cl-single-precision-constant -cl-fast-relaxed-math -cl-finite-math-only -cl-mad-enable -cl-no-signed-zeros";
#endif
    }
  }

  template <>
  void device_t<OpenCL>::setCompiler(const std::string &compiler_){
    compiler = compiler_;
  }

  template <>
  void device_t<OpenCL>::setCompilerEnvScript(const std::string &compilerEnvScript_){
    compilerEnvScript = compilerEnvScript_;
  }

  template <>
  void device_t<OpenCL>::setCompilerFlags(const std::string &compilerFlags_){
    compilerFlags = compilerFlags_;
  }
    
  template <>
  std::string& device_t<OpenCL>::getCompiler(){
    return compiler;
  }

  template <>
  std::string& device_t<OpenCL>::getCompilerEnvScript(){
    return compilerEnvScript;
  }

  template <>
  std::string& device_t<OpenCL>::getCompilerFlags(){
    return compilerFlags;
  }

  template <>
  void device_t<OpenCL>::flush(){
    clFlush(*((cl_command_queue*) dev->currentStream));
  }

  template <>
  void device_t<OpenCL>::finish(){
    clFinish(*((cl_command_queue*) dev->currentStream));
  }

  template <>
  stream device_t<OpenCL>::genStream(){
    OCCA_EXTRACT_DATA(OpenCL, Device);
    cl_int error;

    cl_command_queue *retStream = new cl_command_queue;

    *retStream = clCreateCommandQueue(data_.context, data_.deviceID, CL_QUEUE_PROFILING_ENABLE, &error);
    OCCA_CL_CHECK("Device: genStream", error);

    return retStream;
  }

  template <>
  void device_t<OpenCL>::freeStream(stream s){
    OCCA_CL_CHECK("Device: freeStream",
                  clReleaseCommandQueue( *((cl_command_queue*) s) ));
    delete (cl_command_queue*) s;
  }

  template <>
  tag device_t<OpenCL>::tagStream(){
    cl_command_queue &stream = *((cl_command_queue*) dev->currentStream);

    tag ret;

#ifdef CL_VERSION_1_2
    clEnqueueMarkerWithWaitList(stream, 0, NULL, &(ret.clEvent));
#else
    clEnqueueMarker(stream, &(ret.clEvent));
#endif

    return ret;
  }

  template <>
  double device_t<OpenCL>::timeBetween(const tag &startTag, const tag &endTag){
    cl_command_queue &stream = *((cl_command_queue*) dev->currentStream);
    cl_ulong start, end;

    clFinish(stream);

    OCCA_CL_CHECK ("Device: Time Between Tags (Start)",
                   clGetEventProfilingInfo(startTag.clEvent  ,
                                           CL_PROFILING_COMMAND_END,
                                           sizeof(cl_ulong),
                                           &start, NULL) );

    OCCA_CL_CHECK ("Device: Time Between Tags (End)",
                   clGetEventProfilingInfo(endTag.clEvent  ,
                                           CL_PROFILING_COMMAND_START,
                                           sizeof(cl_ulong),
                                           &end, NULL) );

    clReleaseEvent(startTag.clEvent);
    clReleaseEvent(endTag.clEvent);

    return (double) (1.0e-9 * (double)(end - start));
  }

  template <>
  kernel_v* device_t<OpenCL>::buildKernelFromSource(const std::string &filename,
                                                   const std::string &functionName,
                                                   const kernelInfo &info_){
    OCCA_EXTRACT_DATA(OpenCL, Device);

    kernel_v *k = new kernel_t<OpenCL>;

    k->dev  = dev;
    k->data = new OpenCLKernelData_t;

    OpenCLKernelData_t &kData_ = *((OpenCLKernelData_t*) k->data);

    kData_.platform = data_.platform;
    kData_.device   = data_.device;

    kData_.platformID = data_.platformID;
    kData_.deviceID   = data_.deviceID;
    kData_.context    = data_.context;

    k->buildFromSource(filename, functionName, info_);
    return k;
  }

  template <>
  kernel_v* device_t<OpenCL>::buildKernelFromBinary(const std::string &filename,
                                                    const std::string &functionName){
    OCCA_EXTRACT_DATA(OpenCL, Device);

    kernel_v *k = new kernel_t<OpenCL>;

    k->dev  = dev;
    k->data = new OpenCLKernelData_t;

    OpenCLKernelData_t &kData_ = *((OpenCLKernelData_t*) k->data);

    kData_.platform = data_.platform;
    kData_.device   = data_.device;

    kData_.platformID = data_.platformID;
    kData_.deviceID   = data_.deviceID;
    kData_.context    = data_.context;

    k->buildFromBinary(filename, functionName);
    return k;
  }

  template <>
  memory_v* device_t<OpenCL>::malloc(const uintptr_t bytes,
                                     void *source){
    OCCA_EXTRACT_DATA(OpenCL, Device);

    memory_v *mem = new memory_t<OpenCL>;
    cl_int error;

    mem->dev    = dev;
    mem->handle = new cl_mem;
    mem->size   = bytes;

    if(source == NULL)
      *((cl_mem*) mem->handle) = clCreateBuffer(data_.context,
                                                CL_MEM_READ_WRITE,
                                                bytes, NULL, &error);
    else
      *((cl_mem*) mem->handle) = clCreateBuffer(data_.context,
                                                CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                                bytes, source, &error);

    OCCA_CL_CHECK("Device: malloc", error);

    return mem;
  }

  template <>
  void device_t<OpenCL>::free(){
    OCCA_EXTRACT_DATA(OpenCL, Device);

    OCCA_CL_CHECK("Device: Freeing Context",
                  clReleaseContext(data_.context) );

    delete (OpenCLDeviceData_t*) data;
  }

  template <>
  int device_t<OpenCL>::simdWidth(){
    if(simdWidth_)
      return simdWidth_;

    OCCA_EXTRACT_DATA(OpenCL, Device);

    cl_device_type dBuffer;
    bool isGPU;

    const int bSize = 8192;
    char buffer[bSize + 1];
    buffer[bSize] = '\0';

    OCCA_CL_CHECK("Device: DEVICE_TYPE",
                  clGetDeviceInfo(data_.deviceID, CL_DEVICE_TYPE, sizeof(dBuffer), &dBuffer, NULL));

    OCCA_CL_CHECK("Device: DEVICE_VENDOR",
                  clGetDeviceInfo(data_.deviceID, CL_DEVICE_VENDOR, bSize, buffer, NULL));

    if(dBuffer & CL_DEVICE_TYPE_CPU)
      isGPU = false;
    else if(dBuffer & CL_DEVICE_TYPE_GPU)
      isGPU = true;
    else{
      OCCA_CHECK(false);
    }

    if(isGPU){
      std::string vendor = buffer;
      if(vendor.find("NVIDIA") != std::string::npos)
        simdWidth_ = 32;
      else if((vendor.find("AMD")                    != std::string::npos) ||
              (vendor.find("Advanced Micro Devices") != std::string::npos))
        simdWidth_ = 64;
      else if(vendor.find("Intel") != std::string::npos)   // <> Need to check for Xeon Phi
        simdWidth_ = OCCA_SIMD_WIDTH;
      else{
        OCCA_CHECK(false);
      }
    }
    else
      simdWidth_ = OCCA_SIMD_WIDTH;

    return simdWidth_;
  }
  //==================================


  //---[ Error Handling ]-------------
  std::string openclError(int e){
    switch(e){
    case CL_SUCCESS:                                   return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND:                          return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE:                      return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE:                    return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:             return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:                          return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:                        return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:              return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:                          return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:                     return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:                return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:                     return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:                               return "CL_MAP_FAILURE";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:              return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case CL_INVALID_VALUE:                             return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE:                       return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM:                          return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE:                            return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT:                           return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES:                  return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE:                     return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR:                          return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT:                        return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:           return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE:                        return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER:                           return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY:                            return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS:                     return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM:                           return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE:                return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME:                       return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION:                 return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL:                            return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX:                         return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE:                         return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE:                          return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS:                       return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION:                    return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE:                   return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE:                    return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET:                     return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST:                   return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT:                             return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION:                         return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT:                         return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE:                       return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL:                         return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE:                  return "CL_INVALID_GLOBAL_WORK_SIZE";
    case CL_INVALID_PROPERTY:                          return "CL_INVALID_PROPERTY";
    default:                                           return "UNKNOWN ERROR";
    };
  }
  //==================================
};

#endif
