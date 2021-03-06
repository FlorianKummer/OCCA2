#ifndef OCCA_OPENMP_HEADER
#define OCCA_OPENMP_HEADER

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <fcntl.h>

#include "occaBase.hpp"

#include "occaKernelDefines.hpp"

#if   OCCA_OS == LINUX_OS
#  include <dlfcn.h>
#elif OCCA_OS == OSX_OS
#  include <dlfcn.h>
#else
#  include <windows.h>
#endif

namespace occa {
  //---[ Data Structs ]---------------
  struct OpenMPKernelData_t {
    void *dlHandle, *handle;
  };
  //==================================


  //---[ Kernel ]---------------------
  template <>
  kernel_t<OpenMP>::kernel_t();

  template <>
  kernel_t<OpenMP>::kernel_t(const kernel_t &k);

  template <>
  kernel_t<OpenMP>& kernel_t<OpenMP>::operator = (const kernel_t<OpenMP> &k);

  template <>
  kernel_t<OpenMP>::kernel_t(const kernel_t<OpenMP> &k);

  template <>
  kernel_t<OpenMP>* kernel_t<OpenMP>::buildFromSource(const std::string &filename,
                                                      const std::string &functionName_,
                                                      const kernelInfo &info_);

  template <>
  kernel_t<OpenMP>* kernel_t<OpenMP>::buildFromBinary(const std::string &filename,
                                                      const std::string &functionName_);

  template <>
  int kernel_t<OpenMP>::preferredDimSize();

  template <>
  double kernel_t<OpenMP>::timeTaken();

  template <>
  void kernel_t<OpenMP>::free();
  //==================================


  //---[ Memory ]---------------------
  template <>
  memory_t<OpenMP>::memory_t();

  template <>
  memory_t<OpenMP>::memory_t(const memory_t &m);

  template <>
  memory_t<OpenMP>& memory_t<OpenMP>::operator = (const memory_t &m);

  template <>
  void memory_t<OpenMP>::copyFrom(const void *source,
                                  const uintptr_t bytes,
                                  const uintptr_t offset);

  template <>
  void memory_t<OpenMP>::copyFrom(const memory_v *source,
                                  const uintptr_t bytes,
                                  const uintptr_t destOffset,
                                  const uintptr_t srcOffset);

  template <>
  void memory_t<OpenMP>::copyTo(void *dest,
                                const uintptr_t bytes,
                                const uintptr_t destOffset);

  template <>
  void memory_t<OpenMP>::copyTo(memory_v *dest,
                                const uintptr_t bytes,
                                const uintptr_t srcOffset,
                                const uintptr_t offset);

  template <>
  void memory_t<OpenMP>::asyncCopyFrom(const void *source,
                                       const uintptr_t bytes,
                                       const uintptr_t destOffset);

  template <>
  void memory_t<OpenMP>::asyncCopyFrom(const memory_v *source,
                                       const uintptr_t bytes,
                                       const uintptr_t srcOffset,
                                       const uintptr_t offset);

  template <>
  void memory_t<OpenMP>::asyncCopyTo(void *dest,
                                     const uintptr_t bytes,
                                     const uintptr_t offset);

  template <>
  void memory_t<OpenMP>::asyncCopyTo(memory_v *dest,
                                     const uintptr_t bytes,
                                     const uintptr_t destOffset,
                                     const uintptr_t srcOffset);

  template <>
  void memory_t<OpenMP>::free();
  //==================================


  //---[ Device ]---------------------
  template <>
  device_t<OpenMP>::device_t();

  template <>
  device_t<OpenMP>::device_t(const device_t<OpenMP> &k);

  template <>
  device_t<OpenMP>::device_t(const int arg1, const int arg2);

  template <>
  device_t<OpenMP>& device_t<OpenMP>::operator = (const device_t<OpenMP> &k);

  template <>
  void device_t<OpenMP>::setup(const int arg1, const int arg2);

  template <>
  void device_t<OpenMP>::getEnvironmentVariables();

  template <>
  void device_t<OpenMP>::setCompiler(const std::string &compiler_);

  template <>
  void device_t<OpenMP>::setCompilerEnvScript(const std::string &compilerEnvScript_);

  template <>
  void device_t<OpenMP>::setCompilerFlags(const std::string &compilerFlags_);

  template <>
  void device_t<OpenMP>::flush();

  template <>
  void device_t<OpenMP>::finish();

  template <>
  stream device_t<OpenMP>::genStream();

  template <>
  void device_t<OpenMP>::freeStream(stream s);

  template <>
  tag device_t<OpenMP>::tagStream();

  template <>
  double device_t<OpenMP>::timeBetween(const tag &startTag, const tag &endTag);

  template <>
  kernel_v* device_t<OpenMP>::buildKernelFromSource(const std::string &filename,
                                                    const std::string &functionName_,
                                                    const kernelInfo &info_);

  template <>
  kernel_v* device_t<OpenMP>::buildKernelFromBinary(const std::string &filename,
                                                    const std::string &functionName_);

  template <>
  memory_v* device_t<OpenMP>::malloc(const uintptr_t bytes,
                                     void *source);

  template <>
  void device_t<OpenMP>::free();

  template <>
  int device_t<OpenMP>::simdWidth();
  //==================================

#include "operators/occaFunctionPointerTypeDefs.hpp"
#include "operators/occaOpenMPKernelOperators.hpp"

};

#endif
