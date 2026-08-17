#pragma once
#include "Core/GUID.h"

namespace Gleam {

GSTRUCT(AssetHeader, "ADBF5512-90F9-4F59-B4B4-E2834DA8C731", Serializable, Version(1))
{
	GFIELD("7AE4E1DC-520F-455F-8773-0F0F708A36D9", Serializable)
	Guid typeGuid;
	
	GFIELD("C89824CA-F29B-4136-8E80-68F77E48CAB3", Serializable)
	uint32_t blobCount = 0;

	GFIELD("66B7D442-3FC1-4DFB-967F-E0E06B847BEF", Serializable)
	uint64_t nameOffset = 0;

	GFIELD("F574C093-AE0B-4750-86B1-11155CE4C178", Serializable)
	uint64_t nameSize = 0;

	GFIELD("78770ABA-6737-441A-86EA-AAD59E63E38D", Serializable)
	uint64_t dataTableOffset = 0;

	GFIELD("F51438D7-6B17-4BA1-A672-2C26AAC926A4", Serializable)
	uint64_t dataTableSize = 0;

	GFIELD("6E03A27B-6E62-4ED5-9986-8AC1DCE7CB96", Serializable)
	uint64_t metadataOffset = 0;

	GFIELD("A5123BE6-E747-423C-B829-6485D6B4A268", Serializable)
	uint64_t metadataSize = 0;

	GFIELD("09C8BCBF-F3D9-4C68-B5CE-135FAEB5AF5A", Serializable)
	uint64_t bulkDataOffset = 0;
};

} // namespace Gleam
