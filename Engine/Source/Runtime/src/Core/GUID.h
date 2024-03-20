#pragma once

namespace Gleam {

class Guid
{
public:

	static Guid NewGuid();

	Guid() = default;
	Guid(const TStringView str);

	operator TString() const;

	bool operator==(const Guid &other) const;
	bool operator!=(const Guid &other) const;
    
    const TArray<uint8_t, 16>& GetBytes() const { return mBytes; }

private:

	union
	{
		struct
		{
			uint32_t mData1;
			uint16_t mData2;
			uint16_t mData3;
			uint8_t mData4[8];
		};
		TArray<uint8_t, 16> mBytes = { 0 };
	};

};

static const Guid InvalidGuid = Guid();

} // namespace Gleam

