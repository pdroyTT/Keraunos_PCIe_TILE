/***************************************************************************
 * Copyright 1996-2025 Synopsys, Inc.
 *
 * This Synopsys software and all associated documentation are proprietary
 * to Synopsys, Inc. and may only be used pursuant to the terms and
 * conditions of a written license agreement with Synopsys, Inc.
 * All other use, reproduction, modification, or distribution of the
 * Synopsys software or the associated documentation is strictly prohibited.
 ***************************************************************************/
 

/***************************************************************************
 * Generated snippet, used for detecting user edits.
 * CHECKSUM:151daa6e824bec76c258cf1bee188a1b02ed47ae
 ***************************************************************************/
 
#pragma once

#include "scml2_coverage.h"

#include "Keranous_pcie_tileCovermodelBase.h"



class Keranous_pcie_tileCovermodel
  : public Keranous_pcie_tileCovermodelBase
{
public:
 Keranous_pcie_tileCovermodel(const std::string& test_name, keraunos::pcie::KeraunosPcieTile& t)
  : Keranous_pcie_tileCovermodelBase(test_name, t)
 {
#ifdef SNPS_SLS_VP_COVERAGE
#endif
 }

 virtual ~Keranous_pcie_tileCovermodel() {
  this->write_log();
 }
};

