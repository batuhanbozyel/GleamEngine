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
	
	TArray<uint8_t, 16> mBytes = {0}; 

};

static const Guid InvalidGuid = Guid();

} // namespace Gleam

