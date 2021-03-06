#ifndef OCCA_DEFINES_HEADER
#define OCCA_DEFINES_HEADER

#include "ocl_preprocessor.hpp"

#ifndef LINUX_OS
#  define LINUX_OS 1
#endif

#ifndef OSX_OS
#  define OSX_OS 2
#endif

#ifndef WINDOWS_OS
#  define WINDOWS_OS 4
#endif

#ifndef OCCA_OS
#  ifdef WIN32 || WIN64
#    define OCCA_OS WINDOWS_OS
#    define __PRETTY_FUNCTION__    __FUNCTION__
#  elif __APPLE__
#    define OCCA_OS OSX_OS
#  else
#    define OCCA_OS LINUX_OS
#  endif
#endif

#if defined(__x86_64__) || defined(_M_X64) // 64 Bit
#  define OCCA_64_BIT 1
#  define OCCA_32_BIT 0
#elif defined(__i386) || defined(_M_IX86) // 32 Bit
#  define OCCA_64_BIT 0
#  define OCCA_32_BIT 1
#elif defined(__ia64) || defined(__itanium__) || defined(_A_IA64) // Itanium
#  define OCCA_64_BIT 1
#  define OCCA_32_BIT 0
#endif

//---[ Checks and Info ]----------------
#if OCCA_CHECK_ENABLED
#  define OCCA_CHECK2( _expr , file , line , func )                     \
  do {                                                                  \
    uintptr_t expr = (_expr);                                              \
    if( !expr ){                                                        \
      std::cout << '\n'                                                 \
                << "---[ Error ]--------------------------------------------\n" \
                << "    File     : " << file << '\n'                    \
                << "    Function : " << func << '\n'                    \
                << "    Line     : " << line << '\n'                    \
                << "========================================================\n"; \
      throw 1;                                                          \
    }                                                                   \
  } while(0)
#  define OCCA_CHECK( _expr ) OCCA_CHECK2( _expr , __FILE__ , __LINE__ , __PRETTY_FUNCTION__)
#else
#  define OCCA_CHECK( _expr )
#endif

#if OCCA_OS == OSX_OS
#  define OCCA_MEM_ALIGN 16
#else
#  define OCCA_MEM_ALIGN 64
#endif

#define OCCA_SIMD_WIDTH 8

#define OCCA_MAX_ARGS 50
//======================================


//---[ Base ]---------------------------
#define OCCA_KERNEL_ARG_CONSTRUCTOR(TYPE)       \
  inline kernelArg(const TYPE &arg_){           \
    arg.TYPE##_ = arg_;                         \
      size = sizeof(TYPE);                      \
                                                \
      pointer = false;                          \
  }

#define OCCA_KERNEL_ARG_CONSTRUCTOR_ALIAS(TYPE, ALIAS)  \
  inline kernelArg(const TYPE &arg_){                   \
    arg.ALIAS##_ = arg_;                                \
      size = sizeof(TYPE);                              \
                                                        \
      pointer = false;                                  \
  }

#define OCCA_EXTRACT_DATA(MODE, CLASS)                          \
  MODE##CLASS##Data_t &data_ = *((MODE##CLASS##Data_t*) data);
//======================================


//---[ Pthreads ]-----------------------
//======================================


//---[ OpenMP ]-------------------------
//======================================


//---[ OpenCL ]-------------------------
#if OCCA_CHECK_ENABLED
;
#  define OCCA_CL_CHECK( _str , _statement ) OCCA_CL_CHECK2( _str , _statement , __FILE__ , __LINE__ )
#  define OCCA_CL_CHECK2( _str , _statement , file , line )             \
  do {                                                                  \
    cl_int _error = _statement;                                         \
    if(_error){                                                         \
      _error = _error < 0  ? _error : -_error;                          \
      _error = _error < 65 ? _error : 15;                               \
                                                                        \
      std::cout << "Error\n"                                            \
                << "    File    : " << file << '\n'                     \
                << "    Line    : " << line << '\n'                     \
                << "    Error   : OpenCL Error [ " << _error << " ]: " << openclError(_error) << '\n' \
                << "    Message : " << _str << '\n';                    \
      throw 1;                                                          \
    }                                                                   \
  } while(0);

#else
#  define OCCA_CL_CHECK( _str , _statement ) do { _statement; } while(0);
#endif
//======================================


//---[ CUDA ]---------------------------
#if OCCA_CHECK_ENABLED
#  define OCCA_CUDA_CHECK( _str , _statement ) OCCA_CUDA_CHECK2( _str , _statement , __FILE__ , __LINE__ )
#  define OCCA_CUDA_CHECK2( _str , _statement , file , line )           \
  do {                                                                  \
    CUresult errorCode = _statement;                                    \
    if(errorCode){                                                      \
      std::cout << "Error\n"                                            \
                << "    File    : " << file << '\n'                     \
                << "    Line    : " << line << '\n'                     \
                << "    Error   : CUDA Error [ " << errorCode << " ]: " << occa::cudaError(errorCode) << '\n' \
                << "    Message : " << _str << '\n';                    \
      throw 1;                                                          \
    }                                                                   \
  } while(0);
#else
#  define OCCA_CUDA_CHECK( _str , _statement ) do { _statement; } while(0);
#endif
//======================================


//---[ COI ]----------------------------
#if OCCA_CHECK_ENABLED
#  define OCCA_COI_CHECK( _str , _statement ) OCCA_COI_CHECK2( _str , _statement , __FILE__ , __LINE__ )
#  define OCCA_COI_CHECK2( _str , _statement , file , line )            \
  do {                                                                  \
    COIRESULT errorCode = _statement;                                   \
    if(errorCode != COI_SUCCESS){                                       \
      std::cout << "Error\n"                                            \
                << "    File    : " << file << '\n'                     \
                << "    Line    : " << line << '\n'                     \
                << "    Error   : " << occa::coiError(errorCode) << '\n' \
                << "    Message : " << _str << '\n';                    \
      throw 1;                                                          \
    }                                                                   \
  } while(0);
#else
#  define OCCA_COI_CHECK( _str , _statement ) do { _statement; } while(0);
#endif
//======================================
#endif
