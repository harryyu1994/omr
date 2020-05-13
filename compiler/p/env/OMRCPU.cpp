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

#include "env/CPU.hpp"
#include "env/CompilerEnv.hpp"
#include "env/Processors.hpp"
#include "infra/Assert.hpp"

bool
OMR::Power::CPU::getPPCSupportsVMX()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return false;

   return self()->supportsFeature(OMR_FEATURE_PPC_HAS_ALTIVEC);
   }

bool
OMR::Power::CPU::getPPCSupportsVSX()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return false;

   return self()->supportsFeature(OMR_FEATURE_PPC_HAS_VSX);
   }

bool
OMR::Power::CPU::getPPCSupportsAES()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return false;

   return self()->supportsFeature(OMR_FEATURE_PPC_HAS_ALTIVEC) && self()->isAtLeast(OMR_PROCESSOR_PPC_P8) && self()->supportsFeature(OMR_FEATURE_PPC_HAS_VSX);
   }

bool
OMR::Power::CPU::getPPCSupportsTM()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return false;

   return self()->supportsFeature(OMR_FEATURE_PPC_HTM);
   }

bool
OMR::Power::CPU::hasPopulationCountInstruction()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return false;

#if defined(J9OS_I5)
   return ans;
#endif

   return self()->isAtLeast(OMR_PROCESSOR_PPC_P7);
   }

bool
OMR::Power::CPU::supportsDecimalFloatingPoint()
   {
   if (TR::Compiler->omrPortLib == NULL)
      return false;
   
   return self()->supportsFeature(OMR_FEATURE_PPC_HAS_DFP);
   }

bool
OMR::Power::CPU::getPPCis64bit()
   {
   if (TR::Compiler->omrPortLib == NULL)
      {
      TR_ASSERT(self()->id() >= TR_FirstPPCProcessor && self()->id() <= TR_LastPPCProcessor, "Not a valid PPC Processor Type");
      return self()->id() >= TR_FirstPPC64BitProcessor;
      }

   TR_ASSERT(self()->isAtLeast(OMR_PROCESSOR_PPC_FIRST) && self()->isAtMost(OMR_PROCESSOR_PPC_LAST), "Not a valid PPC Processor Type");
   TR_ASSERT_FATAL((self()->id() >= TR_FirstPPCHwCopySignProcessor) == (self()->isAtLeast(OMR_PROCESSOR_PPC_HW_COPY_SIGN_FIRST)), "getPPCis64bit");
   return self()->isAtLeast(OMR_PROCESSOR_PPC_FIRST);
   }

bool 
OMR::Power::CPU::getSupportsHardwareSQRT()
   {
   if (TR::Compiler->omrPortLib == NULL)
      {
      TR_ASSERT(self()->id() >= TR_FirstPPCProcessor && self()->id() <= TR_LastPPCProcessor, "Not a valid PPC Processor Type");
      return self()->id() >= TR_FirstPPCHwSqrtProcessor;
      }

   TR_ASSERT(self()->isAtLeast(OMR_PROCESSOR_PPC_FIRST) && self()->isAtMost(OMR_PROCESSOR_PPC_LAST), "Not a valid PPC Processor Type");
   TR_ASSERT_FATAL((self()->id() >= TR_FirstPPCHwCopySignProcessor) == (self()->isAtLeast(OMR_PROCESSOR_PPC_HW_COPY_SIGN_FIRST)), "getSupportsHardwareSQRT");
   return self()->isAtLeast(OMR_PROCESSOR_PPC_HW_SQRT_FIRST);
   }

bool
OMR::Power::CPU::getSupportsHardwareRound()
   {
   if (TR::Compiler->omrPortLib == NULL)
      {
      TR_ASSERT(self()->id() >= TR_FirstPPCProcessor && self()->id() <= TR_LastPPCProcessor, "Not a valid PPC Processor Type");
      return self()->id() >= TR_FirstPPCHwRoundProcessor;
      }

   TR_ASSERT(self()->isAtLeast(OMR_PROCESSOR_PPC_FIRST) && self()->isAtMost(OMR_PROCESSOR_PPC_LAST), "Not a valid PPC Processor Type");
   TR_ASSERT_FATAL((self()->id() >= TR_FirstPPCHwCopySignProcessor) == (self()->isAtLeast(OMR_PROCESSOR_PPC_HW_COPY_SIGN_FIRST)), "getSupportsHardwareRound");
   return self()->isAtLeast(OMR_PROCESSOR_PPC_HW_ROUND_FIRST);
   }
   
bool
OMR::Power::CPU::getSupportsHardwareCopySign()
   {
   if (TR::Compiler->omrPortLib == NULL)
      {
      TR_ASSERT(self()->id() >= TR_FirstPPCProcessor && self()->id() <= TR_LastPPCProcessor, "Not a valid PPC Processor Type");
      return self()->id() >= TR_FirstPPCHwCopySignProcessor;
      }

   TR_ASSERT(self()->isAtLeast(OMR_PROCESSOR_PPC_FIRST) && self()->isAtMost(OMR_PROCESSOR_PPC_LAST), "Not a valid PPC Processor Type");
   TR_ASSERT_FATAL((self()->id() >= TR_FirstPPCHwCopySignProcessor) == (self()->isAtLeast(OMR_PROCESSOR_PPC_HW_COPY_SIGN_FIRST)), "getSupportsHardwareCopySign");
   return self()->isAtLeast(OMR_PROCESSOR_PPC_HW_COPY_SIGN_FIRST);
   }

bool
OMR::Power::CPU::supportsTransactionalMemoryInstructions()
   {
   return self()->getPPCSupportsTM();
   }

bool
OMR::Power::CPU::isTargetWithinIFormBranchRange(intptr_t targetAddress, intptr_t sourceAddress)
   {
   intptr_t range = targetAddress - sourceAddress;
   return range <= self()->maxIFormBranchForwardOffset() &&
          range >= self()->maxIFormBranchBackwardOffset();
   }