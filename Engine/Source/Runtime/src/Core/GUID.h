#pragma once

namespace Gleam {

class Guid
{
    friend struct std::hash<Gleam::Guid>;
    
public:

	static Guid NewGuid();

	Guid() = default;
	Guid(const TStringView str);

	operator TString() const;
    TString ToString() const
    {
        return TString(*this);
    }

	bool operator==(const Guid& other) const;
	bool operator!=(const Guid& other) const;
    
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

template <>
struct std::hash<Gleam::Guid>
{
    size_t operator()(const Gleam::Guid& guid) const
    {
        size_t hash = 0;
        Gleam::hash_combine(hash, guid.mData1);
        Gleam::hash_combine(hash, guid.mData2);
        Gleam::hash_combine(hash, guid.mData3);
        
        uint64_t data4;
        memcpy(&data4, guid.mData4, sizeof(uint64_t));
        Gleam::hash_combine(hash, data4);
        return hash;
    }
};
