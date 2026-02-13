// Disable struct alignment
#if defined(_MSC_VER)
#pragma warning(disable : 4103)
#pragma pack(push, packing)
#pragma pack(1)
#define PACK_STRUCT
#elif defined(__GNUC__)
#define PACK_STRUCT __attribute__((packed))
#else
#error compiler not supported
#endif

#include <stdint.h>

namespace extractor
{
	struct ZIPFileDataDescriptor
	{
		uint32_t CRC32;
		uint32_t CompressedSize;
		uint32_t UncompressedSize;
	} PACK_STRUCT;

	struct ZIPFileHeader
	{
		uint32_t Sig; // 'PK0304' little endian (0x04034b50)
		int16_t VersionToExtract;
		int16_t GeneralBitFlag;
		int16_t CompressionMethod;
		int16_t LastModFileTime;
		int16_t LastModFileDate;
		ZIPFileDataDescriptor DataDescriptor;
		int16_t FilenameLength;
		int16_t ExtraFieldLength;
		// filename (variable size)
		// extra field (variable size )
	} PACK_STRUCT;

	struct ZIPFileCentralDirFileHeader
	{
		uint32_t Sig; // 'PK0102' (0x02014b50)
		uint16_t VersionMadeBy;
		uint16_t VersionToExtract;
		uint16_t GeneralBitFlag;
		uint16_t CompressionMethod;
		uint16_t LastModFileTime;
		uint16_t LastModFileDate;
		uint32_t CRC32;
		uint32_t CompressedSize;
		uint32_t UncompressedSize;
		uint16_t FilenameLength;
		uint16_t ExtraFieldLength;
		uint16_t FileCommentLength;
		uint16_t DiskNumberStart;
		uint16_t InternalFileAttributes;
		uint32_t ExternalFileAttributes;
		uint32_t RelativeOffsetOfLocalHeader;

		// filename (variable size)
		// extra field (variable size)
		// file comment (variable size)

	} PACK_STRUCT;

	struct ZIPFileCentralDirEnd
	{
		uint32_t Sig;           // 'PK0506' end_of central dir signature			// (0x06054b50)
		uint16_t NumberDisk;    // number of this disk
		uint16_t NumberStart;   // number of the disk with the start of the central directory
		uint16_t TotalDisk;     // total number of entries in the central dir on this disk
		uint16_t TotalEntries;  // total number of entries in the central dir
		uint32_t Size;          // size of the central directory
		uint32_t Offset;        // offset of start of centraldirectory with respect to the starting disk number
		uint16_t CommentLength; // zipfile comment length
					   // zipfile comment (variable size)
	} PACK_STRUCT;

	struct ZipFileExtraHeader
	{
		int16_t ID;
		int16_t Size;
	} PACK_STRUCT;

	struct ZipFileAESExtraData
	{
		int16_t Version;
		uint8_t Vendor[2];
		uint8_t EncryptionStrength;
		int16_t CompressionMode;
	} PACK_STRUCT;

	enum GZIP_FLAGS
	{
		GZF_TEXT_DAT = 1,
		GZF_CRC16 = 2,
		GZF_EXTRA_FIELDS = 4,
		GZF_FILE_NAME = 8,
		GZF_COMMENT = 16
	};

	struct GZIPMemberHeader
	{
		uint16_t sig;              // 0x8b1f
		uint8_t compressionMethod; // 8 = deflate
		uint8_t flags;
		uint32_t time;
		uint8_t extraFlags; // slow compress = 2, fast compress = 4
		uint8_t operatingSystem;
	} PACK_STRUCT;
};

// Get back to the default alignment
#if defined(_MSC_VER)
#pragma pack(pop, packing)
#endif

#undef PACK_STRUCT
