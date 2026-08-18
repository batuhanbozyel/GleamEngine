#pragma once
#include "Core/GUID.h"
#include "Container/Array.h"
#include "Container/BinaryBuffer.h"

namespace Gleam {

GSTRUCT(AssetDataTable, "91170B46-96F7-48CE-9B16-9946705CF8C6", Serializable)
{
	GFIELD("27E3C612-F302-4E42-96D8-0447D272AFAD", Serializable)
	TArray<BufferRange> blobs;
};

GSTRUCT(AssetHeader, "ADBF5512-90F9-4F59-B4B4-E2834DA8C731", Serializable, Version(1))
{
	GFIELD("7AE4E1DC-520F-455F-8773-0F0F708A36D9", Serializable)
	Guid typeGuid;
	
	GFIELD("C89824CA-F29B-4136-8E80-68F77E48CAB3", Serializable)
	uint32_t blobCount = 0;

	GFIELD("66B7D442-3FC1-4DFB-967F-E0E06B847BEF", Serializable)
	BufferRange name;

	GFIELD("78770ABA-6737-441A-86EA-AAD59E63E38D", Serializable)
	BufferRange dataTable;

	GFIELD("6E03A27B-6E62-4ED5-9986-8AC1DCE7CB96", Serializable)
	BufferRange metadata;

	GFIELD("09C8BCBF-F3D9-4C68-B5CE-135FAEB5AF5A", Serializable)
	BufferRange bulkData;
};

} // namespace Gleam
