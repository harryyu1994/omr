/*******************************************************************************
 * Copyright (c) 2000, 2020 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "env/CPU.hpp"
#include "env/CompilerEnv.hpp"
#include "env/JitConfig.hpp"
#include "env/ProcessorInfo.hpp"
#include "infra/Flags.hpp"
#include "x/runtime/X86Runtime.hpp"
#include "codegen/CodeGenerator.hpp"

TR::CPU
OMR::X86::CPU::detect(OMRPortLibrary * const omrPortLib)
   {
   if (omrPortLib == NULL)
      return TR::CPU();

   // Only enable the features that compiler currently uses
   uint32_t enabledFeatures [] = {OMR_FEATURE_X86_FPU, OMR_FEATURE_X86_CX8, OMR_FEATURE_X86_CMOV,
                                  OMR_FEATURE_X86_MMX, OMR_FEATURE_X86_SSE, OMR_FEATURE_X86_SSE2,
                                  OMR_FEATURE_X86_SSSE3, OMR_FEATURE_X86_SSE4_1, OMR_FEATURE_X86_POPCNT,
                                  OMR_FEATURE_X86_AESNI, OMR_FEATURE_X86_OSXSAVE, OMR_FEATURE_X86_AVX,
                                  OMR_FEATURE_X86_FMA, OMR_FEATURE_X86_HLE, OMR_FEATURE_X86_RTM};

   OMRPORT_ACCESS_FROM_OMRPORT(omrPortLib);
   OMRProcessorDesc featureMasks;
   memset(featureMasks.features, 0, OMRPORT_SYSINFO_FEATURES_SIZE*sizeof(uint32_t));
   for (size_t i = 0; i < sizeof(enabledFeatures)/sizeof(uint32_t); i++)
      {
      omrsysinfo_processor_set_feature(&featureMasks, enabledFeatures[i], TRUE);
      }

   OMRProcessorDesc processorDescription;
   omrsysinfo_get_processor_description(&processorDescription);
   for (size_t i = 0; i < OMRPORT_SYSINFO_FEATURES_SIZE; i++)
      {
      processorDescription.features[i] &= featureMasks.features[i];
      }

   if (TRUE == omrsysinfo_processor_has_feature(&processorDescription, OMR_FEATURE_X86_OSXSAVE))
      {
      if (((6 & _xgetbv(0)) != 6) || feGetEnv("TR_DisableAVX")) // '6' = mask for XCR0[2:1]='11b' (XMM state and YMM state are enabled)
         {
         // Unset OSXSAVE if not enabled via CR0
         omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_OSXSAVE, FALSE);
         }
      }

   return TR::CPU(processorDescription);
   }

TR_X86CPUIDBuffer *
OMR::X86::CPU::queryX86TargetCPUID()
   {
   static TR_X86CPUIDBuffer *buf = NULL;
   auto jitConfig = TR::JitConfig::instance();

   if (jitConfig && jitConfig->getProcessorInfo() == NULL)
      {
      buf = (TR_X86CPUIDBuffer *) malloc(sizeof(TR_X86CPUIDBuffer));
      if (!buf)
         return 0;
      jitGetCPUID(buf);
      jitConfig->setProcessorInfo(buf);
      }
   else
      {
      if (!buf)
         {
         if (jitConfig && jitConfig->getProcessorInfo())
            {
            buf = (TR_X86CPUIDBuffer *)jitConfig->getProcessorInfo();
            }
         else
            {
            buf = (TR_X86CPUIDBuffer *) malloc(sizeof(TR_X86CPUIDBuffer));

            if (!buf)
               memcpy(buf->_vendorId, "Poughkeepsie", 12); // 12 character dummy string (NIL term not used)

            buf->_processorSignature = 0;
            buf->_brandIdEtc = 0;
            buf->_featureFlags = 0x00000000;
            buf->_cacheDescription.l1instr = 0;
            buf->_cacheDescription.l1data  = 0;
            buf->_cacheDescription.l2      = 0;
            buf->_cacheDescription.l3      = 0;
            }
         }
      }

   return buf;
   }

const char *
OMR::X86::CPU::getX86ProcessorVendorId()
   {
   static char buf[13];

   // Terminate the vendor ID with NULL before returning.
   //
   strncpy(buf, self()->queryX86TargetCPUID()->_vendorId, 12);
   buf[12] = '\0';

   return buf;
   }

uint32_t
OMR::X86::CPU::getX86ProcessorSignature()
   {
   return self()->queryX86TargetCPUID()->_processorSignature;
   }

uint32_t
OMR::X86::CPU::getX86ProcessorFeatureFlags()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return self()->queryX86TargetCPUID()->_featureFlags;

   return self()->_processorDescription.features[0];
   }

uint32_t
OMR::X86::CPU::getX86ProcessorFeatureFlags2()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return self()->queryX86TargetCPUID()->_featureFlags2;

   return self()->_processorDescription.features[1];
   }

uint32_t
OMR::X86::CPU::getX86ProcessorFeatureFlags8()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return self()->queryX86TargetCPUID()->_featureFlags8;

   return self()->_processorDescription.features[3];
   }

bool
OMR::X86::CPU::getSupportsHardwareSQRT()
   {
   return true;
   }

bool
OMR::X86::CPU::testOSForSSESupport()
   {
   return false;
   }

bool
OMR::X86::CPU::supportsTransactionalMemoryInstructions()
   {
   flags32_t processorFeatureFlags8(self()->getX86ProcessorFeatureFlags8());
   return processorFeatureFlags8.testAny(TR_RTM);
   }

bool
OMR::X86::CPU::isGenuineIntel()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return TR::CodeGenerator::getX86ProcessorInfo().isGenuineIntel();

   return self()->isAtLeast(OMR_PROCESSOR_X86_INTEL_FIRST) && self()->isAtMost(OMR_PROCESSOR_X86_INTEL_LAST);
   }

bool
OMR::X86::CPU::isAuthenticAMD()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return TR::CodeGenerator::getX86ProcessorInfo().isAuthenticAMD();

   return self()->isAtLeast(OMR_PROCESSOR_X86_AMD_FIRST) && self()->isAtMost(OMR_PROCESSOR_X86_AMD_LAST);
   }

bool
OMR::X86::CPU::requiresLFence()
   {
   return false;  /* Dummy for now, we may need LFENCE in future processors*/
   }

bool
OMR::X86::CPU::supportsFCOMIInstructions()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return TR::CodeGenerator::getX86ProcessorInfo().isGenuineIntel();

   return self()->supportsFeature(OMR_FEATURE_X86_FPU) || self()->supportsFeature(OMR_FEATURE_X86_CMOV);
   }

bool
OMR::X86::CPU::supportsMFence()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return TR::CodeGenerator::getX86ProcessorInfo().supportsMFence();

   return self()->supportsFeature(OMR_FEATURE_X86_SSE2);
   }

bool
OMR::X86::CPU::supportsLFence()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return TR::CodeGenerator::getX86ProcessorInfo().supportsLFence();

   return self()->supportsFeature(OMR_FEATURE_X86_SSE2);
   }

bool
OMR::X86::CPU::supportsSFence()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return TR::CodeGenerator::getX86ProcessorInfo().supportsSFence();

   return self()->supportsFeature(OMR_FEATURE_X86_SSE2) || self()->supportsFeature(OMR_FEATURE_X86_MMX);
   }

bool
OMR::X86::CPU::prefersMultiByteNOP()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return TR::CodeGenerator::getX86ProcessorInfo().prefersMultiByteNOP();

   return self()->isGenuineIntel() && !self()->is(OMR_PROCESSOR_X86_INTELPENTIUM);
   }

bool
OMR::X86::CPU::supportsAVX()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return TR::CodeGenerator::getX86ProcessorInfo().supportsAVX();

   return self()->supportsFeature(OMR_FEATURE_X86_AVX) && self()->supportsFeature(OMR_FEATURE_X86_OSXSAVE);
   }

bool
OMR::X86::CPU::is(OMRProcessorArchitecture p)
   {
   if (TR::Compiler->omrPortLib == NULL)
      return self()->is_old_api(p);
   self()->is_test(p);

   return (_processorDescription.processor == p);
   }

bool
OMR::X86::CPU::supportsFeature(uint32_t feature)
   {
   if (TR::Compiler->omrPortLib == NULL)
      return self()->supports_feature_old_api(feature);
   self()->supports_feature_test(feature);

   OMRPORT_ACCESS_FROM_OMRPORT(TR::Compiler->omrPortLib);
   return (TRUE == omrsysinfo_processor_has_feature(&_processorDescription, feature));
   }

bool
OMR::X86::CPU::is_old_api(OMRProcessorArchitecture p)
   {
   bool ans = false;
   switch(p)
      {
      case OMR_PROCESSOR_X86_INTELWESTMERE:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelWestmere();
         break;
      case OMR_PROCESSOR_X86_INTELNEHALEM:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelNehalem();
         break;
      case OMR_PROCESSOR_X86_INTELPENTIUM:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelPentium();
         break;
      case OMR_PROCESSOR_X86_INTELP6:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelP6();
         break;
      case OMR_PROCESSOR_X86_INTELPENTIUM4:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelPentium4();
         break;
      case OMR_PROCESSOR_X86_INTELCORE2:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelCore2();
         break;
      case OMR_PROCESSOR_X86_INTELTULSA:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelTulsa();
         break;
      case OMR_PROCESSOR_X86_INTELSANDYBRIDGE:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelSandyBridge();
         break;
      case OMR_PROCESSOR_X86_INTELIVYBRIDGE:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelIvyBridge();
         break;
      case OMR_PROCESSOR_X86_INTELHASWELL:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelHaswell();
         break;
      case OMR_PROCESSOR_X86_INTELBROADWELL:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelBroadwell();
         break;
      case OMR_PROCESSOR_X86_INTELSKYLAKE:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isIntelSkylake();
         break;
      case OMR_PROCESSOR_X86_AMDATHLONDURON:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isAMDAthlonDuron();
         break;
      case OMR_PROCESSOR_X86_AMDOPTERON:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isAMDOpteron();
         break;
      case OMR_PROCESSOR_X86_AMDFAMILY15H:
         ans = TR::CodeGenerator::getX86ProcessorInfo().isAMD15h();
         break;
      default:
         ans = false;
         break;
      }
   return ans;
   }

void
OMR::X86::CPU::is_test(OMRProcessorArchitecture p)
   {
   switch(p)
      {
      case OMR_PROCESSOR_X86_INTELWESTMERE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelWestmere() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELWESTMERE");
         break;
      case OMR_PROCESSOR_X86_INTELNEHALEM:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelNehalem() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELNEHALEM");
         break;
      case OMR_PROCESSOR_X86_INTELPENTIUM:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelPentium() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELPENTIUM");
         break;
      case OMR_PROCESSOR_X86_INTELP6:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelP6() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELP6");
         break;
      case OMR_PROCESSOR_X86_INTELPENTIUM4:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelPentium4() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELPENTIUM4");
         break;
      case OMR_PROCESSOR_X86_INTELCORE2:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelCore2() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELCORE2");
         break;
      case OMR_PROCESSOR_X86_INTELTULSA:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelTulsa() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELTULSA");
         break;
      case OMR_PROCESSOR_X86_INTELSANDYBRIDGE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelSandyBridge() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELSANDYBRIDGE");
         break;
      case OMR_PROCESSOR_X86_INTELIVYBRIDGE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelIvyBridge() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELIVYBRIDGE");
         break;
      case OMR_PROCESSOR_X86_INTELHASWELL:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelHaswell() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELHASWELL");
         break;
      case OMR_PROCESSOR_X86_INTELBROADWELL:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelBroadwell() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELBROADWELL");
         break;
      case OMR_PROCESSOR_X86_INTELSKYLAKE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isIntelSkylake() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_INTELSKYLAKE");
         break;
      case OMR_PROCESSOR_X86_AMDATHLONDURON:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isAMDAthlonDuron() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_AMDATHLONDURON");
         break;
      case OMR_PROCESSOR_X86_AMDOPTERON:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isAMDOpteron() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_AMDOPTERON");
         break;
      case OMR_PROCESSOR_X86_AMDFAMILY15H:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().isAMD15h() == (_processorDescription.processor == p), "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      default:
         TR_ASSERT_FATAL(false, "Unknown processor");
         break;
      }
   return;
   }

bool
OMR::X86::CPU::supports_feature_old_api(uint32_t feature)
   {
   bool supported = false;
   switch(feature)
      {
      case OMR_FEATURE_X86_OSXSAVE:
         supported = TR::CodeGenerator::getX86ProcessorInfo().enabledXSAVE();
         break;
      case OMR_FEATURE_X86_FPU:
         supported = TR::CodeGenerator::getX86ProcessorInfo().hasBuiltInFPU();
         break;
      case OMR_FEATURE_X86_VME:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsVirtualModeExtension();
         break;
      case OMR_FEATURE_X86_DE:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsDebuggingExtension();
         break;
      case OMR_FEATURE_X86_PSE:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsPageSizeExtension();
         break;
      case OMR_FEATURE_X86_TSC:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsRDTSCInstruction();
         break;
      case OMR_FEATURE_X86_MSR:
         supported = TR::CodeGenerator::getX86ProcessorInfo().hasModelSpecificRegisters();
         break;
      case OMR_FEATURE_X86_PAE:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsPhysicalAddressExtension();
         break;
      case OMR_FEATURE_X86_MCE:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsMachineCheckException();
         break;
      case OMR_FEATURE_X86_CX8:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsCMPXCHG8BInstruction();
         break;
      case OMR_FEATURE_X86_CMPXCHG16B:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsCMPXCHG16BInstruction();
         break;
      case OMR_FEATURE_X86_APIC:
         supported = TR::CodeGenerator::getX86ProcessorInfo().hasAPICHardware();
         break;
      case OMR_FEATURE_X86_MTRR:
         supported = TR::CodeGenerator::getX86ProcessorInfo().hasMemoryTypeRangeRegisters();
         break;
      case OMR_FEATURE_X86_PGE:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsPageGlobalFlag();
         break;
      case OMR_FEATURE_X86_MCA:
         supported = TR::CodeGenerator::getX86ProcessorInfo().hasMachineCheckArchitecture();
         break;
      case OMR_FEATURE_X86_CMOV:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsCMOVInstructions();
         break;
      case OMR_FEATURE_X86_PAT:
         supported = TR::CodeGenerator::getX86ProcessorInfo().hasPageAttributeTable();
         break;
      case OMR_FEATURE_X86_PSE_36:
         supported = TR::CodeGenerator::getX86ProcessorInfo().has36BitPageSizeExtension();
         break;
      case OMR_FEATURE_X86_PSN:
         supported = TR::CodeGenerator::getX86ProcessorInfo().hasProcessorSerialNumber();
         break;
      case OMR_FEATURE_X86_CLFSH:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsCLFLUSHInstruction();
         break;
      case OMR_FEATURE_X86_DS:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsDebugTraceStore();
         break;
      case OMR_FEATURE_X86_ACPI:
         supported = TR::CodeGenerator::getX86ProcessorInfo().hasACPIRegisters();
         break;
      case OMR_FEATURE_X86_MMX:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsMMXInstructions();
         break;
      case OMR_FEATURE_X86_FXSR:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsFastFPSavesRestores();
         break;
      case OMR_FEATURE_X86_SSE:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsSSE();
         break;
      case OMR_FEATURE_X86_SSE2:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsSSE2();
         break;
      case OMR_FEATURE_X86_SSE3:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsSSE3();
         break;
      case OMR_FEATURE_X86_SSSE3:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsSSSE3();
         break;
      case OMR_FEATURE_X86_SSE4_1:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsSSE4_1();
         break;
      case OMR_FEATURE_X86_SSE4_2:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsSSE4_2();
         break;
      case OMR_FEATURE_X86_PCLMULQDQ:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsCLMUL();
         break;
      case OMR_FEATURE_X86_AESNI:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsAESNI();
         break;
      case OMR_FEATURE_X86_POPCNT:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsPOPCNT();
         break;
      case OMR_FEATURE_X86_SS:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsSelfSnoop();
         break;
      case OMR_FEATURE_X86_RTM:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsTM();
         break;
      case OMR_FEATURE_X86_HTT:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsHyperThreading();
         break;
      case OMR_FEATURE_X86_HLE:
         supported = TR::CodeGenerator::getX86ProcessorInfo().supportsHLE();
         break;
      case OMR_FEATURE_X86_TM:
         supported = TR::CodeGenerator::getX86ProcessorInfo().hasThermalMonitor();
         break;
      case OMR_FEATURE_X86_AVX:
         break;
      default:
         TR_ASSERT_FATAL(false, "Unknown feature");
         break;
      }
   return supported;
   }

void
OMR::X86::CPU::supports_feature_test(uint32_t feature)
   {
   OMRPORT_ACCESS_FROM_OMRPORT(TR::Compiler->omrPortLib);
   bool ans = (TRUE == omrsysinfo_processor_has_feature(&_processorDescription, feature));

   switch(feature)
      {
      case OMR_FEATURE_X86_OSXSAVE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().enabledXSAVE() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_FPU:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().hasBuiltInFPU() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_VME:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsVirtualModeExtension() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_DE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsDebuggingExtension() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_PSE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsPageSizeExtension() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_TSC:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsRDTSCInstruction() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_MSR:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().hasModelSpecificRegisters() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_PAE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsPhysicalAddressExtension() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_MCE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsMachineCheckException() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_CX8:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsCMPXCHG8BInstruction() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_CMPXCHG16B:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsCMPXCHG16BInstruction() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_APIC:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().hasAPICHardware() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_MTRR:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().hasMemoryTypeRangeRegisters() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_PGE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsPageGlobalFlag() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_MCA:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().hasMachineCheckArchitecture() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_CMOV:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsCMOVInstructions() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_PAT:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().hasPageAttributeTable() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_PSE_36:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().has36BitPageSizeExtension() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_PSN:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().hasProcessorSerialNumber() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_CLFSH:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsCLFLUSHInstruction() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_DS:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsDebugTraceStore() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_ACPI:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().hasACPIRegisters() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_MMX:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsMMXInstructions() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_FXSR:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsFastFPSavesRestores() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_SSE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsSSE() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_SSE2:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsSSE2() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_SSE3:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsSSE3() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_SSSE3:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsSSSE3() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_SSE4_1:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsSSE4_1() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_SSE4_2:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsSSE4_2() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_PCLMULQDQ:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsCLMUL() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_AESNI:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsAESNI() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_POPCNT:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsPOPCNT() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_SS:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsSelfSnoop() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_RTM:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsTM() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_HTT:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsHyperThreading() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_HLE:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().supportsHLE() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_TM:
         TR_ASSERT_FATAL(TR::CodeGenerator::getX86ProcessorInfo().hasThermalMonitor() == ans, "OMR_PROCESSOR_X86_AMDFAMILY15H");
         break;
      case OMR_FEATURE_X86_AVX:
         break;
      default:
         TR_ASSERT_FATAL(false, "Unknown feature");
         break;
      }
   return;
   }

