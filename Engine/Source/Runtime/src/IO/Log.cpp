#include "gpch.h"
#include "Log.h"
#include "Core/Globals.h"

#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
#elif defined(PLATFORM_MACOS)
#include <os/log.h>
#endif

using namespace Gleam;

class Logger::History final
{
public:

	void Write(const Logger* source, Level level, std::time_t time, std::string_view message);

	void Read(uint64_t& cursor, const std::function<void(const Entry&)>& visitor) const;

private:

	struct RecordHeader
	{
		std::time_t time;
		const Logger* source;
		uint32_t length;
		Level level;
	};

	static constexpr size_t kCapacity = 16 * 1024;
	static constexpr uint32_t kPaddingMarker = UINT32_MAX;

	static size_t AlignRecordSize(size_t size);

	static size_t WrapOffset(size_t offset);

	RecordHeader* GetRecord(size_t offset);

	const RecordHeader* GetRecord(size_t offset) const;

	bool IsPadding(size_t offset) const;

	size_t GetRecordSize(size_t offset) const;

	void EvictOldestRecord();

	alignas(RecordHeader) uint8_t mBuffer[kCapacity] = {};
	size_t mHead = 0;
	size_t mTail = 0;
	size_t mUsed = 0;
	uint64_t mFirstSequence = 0;
	uint64_t mRecordCount = 0;
};

constinit Logger::History Logger::mHistory;

void Logger::History::Write(const Logger* source, Level level, std::time_t time, std::string_view message)
{
	const auto length = static_cast<uint32_t>(std::min(message.size(), kCapacity - sizeof(RecordHeader)));
	const size_t recordSize = AlignRecordSize(sizeof(RecordHeader) + length);

	size_t padding = mTail + recordSize > kCapacity ? kCapacity - mTail : 0;
	while (mRecordCount > 0 and mUsed + padding + recordSize > kCapacity)
	{
		EvictOldestRecord();
	}

	if (mRecordCount == 0)
	{
		mHead = 0;
		mTail = 0;
		mUsed = 0;
	}
	else if (padding > 0)
	{
		if (padding >= sizeof(RecordHeader))
		{
			GetRecord(mTail)->length = kPaddingMarker;
		}
		mUsed += padding;
		mTail = 0;
	}

	auto record = GetRecord(mTail);
	*record = RecordHeader{ .time = time, .source = source, .length = length, .level = level };
	std::memcpy(record + 1, message.data(), length);

	mTail = WrapOffset(mTail + recordSize);
	mUsed += recordSize;
	++mRecordCount;
}

void Logger::History::Read(uint64_t& cursor, const std::function<void(const Entry&)>& visitor) const
{
	size_t offset = mHead;
	uint64_t sequence = mFirstSequence;
	const uint64_t endSequence = mFirstSequence + mRecordCount;
	while (sequence < endSequence)
	{
		if (IsPadding(offset))
		{
			offset = 0;
		}
		else
		{
			const auto record = GetRecord(offset);
			if (sequence >= cursor)
			{
				const auto& sourceName = record->source->mName;
				visitor(Entry{
					.level = record->level,
					.time = record->time,
					.source = TStringView(sourceName.data(), sourceName.size()),
					.message = TStringView(reinterpret_cast<const char*>(record + 1), record->length)
				});
			}
			offset = WrapOffset(offset + GetRecordSize(offset));
			++sequence;
		}
	}
	cursor = endSequence;
}

size_t Logger::History::AlignRecordSize(size_t size)
{
	return (size + alignof(RecordHeader) - 1) & ~(alignof(RecordHeader) - 1);
}

size_t Logger::History::WrapOffset(size_t offset)
{
	return offset == kCapacity ? 0 : offset;
}

Logger::History::RecordHeader* Logger::History::GetRecord(size_t offset)
{
	return reinterpret_cast<RecordHeader*>(mBuffer + offset);
}

const Logger::History::RecordHeader* Logger::History::GetRecord(size_t offset) const
{
	return reinterpret_cast<const RecordHeader*>(mBuffer + offset);
}

bool Logger::History::IsPadding(size_t offset) const
{
	return offset + sizeof(RecordHeader) > kCapacity or GetRecord(offset)->length == kPaddingMarker;
}

size_t Logger::History::GetRecordSize(size_t offset) const
{
	return AlignRecordSize(sizeof(RecordHeader) + GetRecord(offset)->length);
}

void Logger::History::EvictOldestRecord()
{
	if (IsPadding(mHead))
	{
		mUsed -= kCapacity - mHead;
		mHead = 0;
	}
	else
	{
		const size_t size = GetRecordSize(mHead);
		mUsed -= size;
		mHead = WrapOffset(mHead + size);
		++mFirstSequence;
		--mRecordCount;
	}
}

Logger::Logger(const TString& name)
    : mName(name)
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        mFileStream = CreateScope<std::ofstream>(FILE_NAME, std::ofstream::out);
    });
    mInstanceCount++;
}

Logger::~Logger()
{
    mInstanceCount--;
    if (mInstanceCount == 0)
    {
        mFileStream.reset();
    }
}

const Logger& Logger::GetCoreLogger()
{
	static Logger sLogger("GLEAM");
	return sLogger;
}

const Logger& Logger::GetClientLogger()
{
	static Logger sLogger(Globals::ProjectName);
	return sLogger;
}

void Logger::RecordHistory(Level lvl, std::time_t time, std::string_view message) const
{
	mHistory.Write(this, lvl, time, message);
}

void Logger::ReadHistory(uint64_t& cursor, const std::function<void(const Entry&)>& visitor)
{
	std::lock_guard<std::mutex> lock(mLogMutex);
	mHistory.Read(cursor, visitor);
}

void Logger::OutputToDebugger(const char* message)
{
#ifdef PLATFORM_WINDOWS
	::OutputDebugStringA(message);
#elif defined(PLATFORM_MACOS)
	os_log(OS_LOG_DEFAULT, "%{public}s", message);
#else
	std::fputs(message, stdout);
	std::fflush(stdout);
#endif
}
