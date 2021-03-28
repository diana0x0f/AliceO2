// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "ITSMFTWorkflow/ClusterReaderSpec.h"
#include "ITSWorkflow/TrackReaderSpec.h"
#include "TPCWorkflow/TrackReaderSpec.h"
#include "TPCWorkflow/PublisherSpec.h"
#include "TPCWorkflow/ClusterSharingMapSpec.h"
#include "FT0Workflow/RecPointReaderSpec.h"
#include "GlobalTrackingWorkflow/TPCITSMatchingSpec.h"
#include "GlobalTrackingWorkflow/TrackWriterTPCITSSpec.h"
#include "Algorithm/RangeTokenizer.h"

#include "ReconstructionDataFormats/GlobalTrackID.h"
#include "DetectorsCommonDataFormats/DetID.h"
#include "CommonUtils/ConfigurableParam.h"
#include "Framework/CompletionPolicy.h"
#include "GPUWorkflow/TPCSectorCompletionPolicy.h"
#include "DetectorsRaw/HBFUtilsInitializer.h"

using namespace o2::framework;
using GID = o2::dataformats::GlobalTrackID;
// ------------------------------------------------------------------

// we need to add workflow options before including Framework/runDataProcessing
void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  // option allowing to set parameters
  std::vector<o2::framework::ConfigParamSpec> options{
    {"use-ft0", o2::framework::VariantType::Bool, false, {"use FT0 in matching"}},
    {"disable-mc", o2::framework::VariantType::Bool, false, {"disable MC propagation even if available"}},
    {"disable-root-input", o2::framework::VariantType::Bool, false, {"disable root-files input reader"}},
    {"disable-root-output", o2::framework::VariantType::Bool, false, {"disable root-files output writer"}},
    {"track-sources", VariantType::String, std::string{GID::ALL}, {"comma-separated list of sources to use"}},
    {"produce-calibration-data", o2::framework::VariantType::Bool, false, {"produce output for TPC vdrift calibration"}},
    {"configKeyValues", VariantType::String, "", {"Semicolon separated key=value strings ..."}}};

  o2::raw::HBFUtilsInitializer::addConfigOption(options);

  std::swap(workflowOptions, options);
}

// the matcher process requires the TPC sector completion to trigger and data on
// all defined routes
void customize(std::vector<o2::framework::CompletionPolicy>& policies)
{
  // the TPC sector completion policy checks when the set of TPC/CLUSTERNATIVE data is complete
  // in addition we require to have input from all other routes
  policies.push_back(o2::tpc::TPCSectorCompletionPolicy("itstpc-track-matcher",
                                                        o2::tpc::TPCSectorCompletionPolicy::Config::RequireAll,
                                                        o2::framework::InputSpec{"cluster", o2::framework::ConcreteDataTypeMatcher{"TPC", "CLUSTERNATIVE"}})());
}

// ------------------------------------------------------------------

#include "Framework/runDataProcessing.h"

WorkflowSpec defineDataProcessing(o2::framework::ConfigContext const& configcontext)
{
  // Update the (declared) parameters if changed from the command line
  o2::conf::ConfigurableParam::updateFromString(configcontext.options().get<std::string>("configKeyValues"));
  // write the configuration used for the workflow
  o2::conf::ConfigurableParam::writeINI("o2matchtpcits-workflow_configuration.ini");

  GID::mask_t alowedSources = GID::getSourcesMask("ITS,TPC,FT0");
  GID::mask_t src = alowedSources & GID::getSourcesMask(configcontext.options().get<std::string>("track-sources"));

  auto useFT0 = configcontext.options().get<bool>("use-ft0");
  auto useMC = !configcontext.options().get<bool>("disable-mc");
  auto disableRootInp = configcontext.options().get<bool>("disable-root-input");
  auto disableRootOut = configcontext.options().get<bool>("disable-root-output");
  auto calib = configcontext.options().get<bool>("produce-calibration-data");

  o2::framework::WorkflowSpec specs;

  std::vector<int> tpcClusSectors = o2::RangeTokenizer::tokenize<int>("0-35");
  std::vector<int> tpcClusLanes = tpcClusSectors;

  if (!disableRootInp) {

    specs.emplace_back(o2::its::getITSTrackReaderSpec(useMC)); // ITS always needed
    specs.emplace_back(o2::itsmft::getITSClusterReaderSpec(useMC, true));

    if (src[GID::TPC]) {
      specs.emplace_back(o2::tpc::getTPCTrackReaderSpec(useMC));
      specs.emplace_back(o2::tpc::getPublisherSpec(o2::tpc::PublisherConf{
                                                     "tpc-native-cluster-reader",
                                                     "tpc-native-clusters.root",
                                                     "tpcrec",
                                                     {"clusterbranch", "TPCClusterNative", "Branch with TPC native clusters"},
                                                     {"clustermcbranch", "TPCClusterNativeMCTruth", "MC label branch"},
                                                     OutputSpec{"TPC", "CLUSTERNATIVE"},
                                                     OutputSpec{"TPC", "CLNATIVEMCLBL"},
                                                     tpcClusSectors,
                                                     tpcClusLanes},
                                                   useMC));
      specs.emplace_back(o2::tpc::getClusterSharingMapSpec());
    }

    if (useFT0) {
      specs.emplace_back(o2::ft0::getRecPointReaderSpec(useMC));
    }
  }

  specs.emplace_back(o2::globaltracking::getTPCITSMatchingSpec(src, useFT0, calib, useMC));

  if (!disableRootOut) {
    specs.emplace_back(o2::globaltracking::getTrackWriterTPCITSSpec(useMC));
  }

  return std::move(specs);
}
