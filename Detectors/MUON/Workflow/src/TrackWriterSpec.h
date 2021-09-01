// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file TrackWriterSpec.h
/// \brief Definition of a data processor to write matched MCH-MID tracks in a root file
///
/// \author Philippe Pillot, Subatech

#ifndef ALICEO2_MUON_TRACKWRITERSPEC_H_
#define ALICEO2_MUON_TRACKWRITERSPEC_H_

#include "Framework/DataProcessorSpec.h"

namespace o2
{
namespace muon
{

framework::DataProcessorSpec getTrackWriterSpec(const char* specName = "TrackWriter",
                                                const char* fileName = "muontracks.root");

} // namespace muon
} // namespace o2

#endif // ALICEO2_MUON_TRACKWRITERSPEC_H_
