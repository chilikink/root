// Authors: Axel Naumann, Philippe Canal, Danilo Piparo

/*************************************************************************
 * Copyright (C) 1995-2016, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "rootcling_impl.h"
#include "RConfigure.h"
#include <ROOT/RConfig.hxx>
#include <cstdlib>
#include <string>

extern "C" {
   R__DLLEXPORT void usedToIdentifyRootClingByDlSym() {}
}

// force compiler to emit symbol for function above
static void (*dlsymaddr)() = &usedToIdentifyRootClingByDlSym;

ROOT::Internal::RootCling::TROOTSYSSetter gROOTSYSSetter;

static const char *GetIncludeDir() {
   const char *rootsys = getenv("ROOTSYS");
   // The environment variable ROOTSYS is expected to be set by SetRootSys().
   assert(rootsys != nullptr);
   static std::string incdir = std::string(rootsys) + "/include";
   return incdir.c_str();
}

static const char *GetEtcDir() {
   const char *rootsys = getenv("ROOTSYS");
   // The environment variable ROOTSYS is expected to be set by SetRootSys().
   assert(rootsys != nullptr);
   static std::string etcdir = std::string(rootsys) + "/etc";
   return etcdir.c_str();
}

int main(int argc, char **argv)
{
   (void) dlsymaddr; // avoid unused variable warning

   ROOT::Internal::RootCling::DriverConfig config{};

   config.fBuildingROOTStage1 = true;
   config.fTROOT__GetIncludeDir = &GetIncludeDir;
   config.fTROOT__GetEtcDir = &GetEtcDir;

   return ROOT_rootcling_Driver(argc, argv, config);
}
