#include "extractZip.h"

#include <cstring>
#include <algorithm>
#include <experimental/filesystem>
#include <SDL_log.h>

#include <zlib.h>

namespace fs = std::experimental::filesystem;

#define FILENAME(p) fs::path(p).filename()
#define EXTENSION(p) fs::path(p).extension()
#define FILENAME_BS(p) FILENAME(p).generic_string()
#define EXTENSION_BS(p) EXTENSION(p).generic_string()

uint32_t getFileSize(std::ifstream &file) {
    uint32_t prevPos = file.tellg();
	file.seekg(0, std::ios_base::end);
    uint32_t size = file.tellg();
	file.seekg(prevPos);

	return size;
}


namespace extractor
{

    bool ZipExtractor::IsFormatSupported(const std::string &filename)
	{
        fs::path ext = EXTENSION(filename);

		return (ext == "zip" || ext == "gz" || ext == "tgz");
	}

	bool ZipExtractor::extractArchive(const std::string &filename, const std::string &destination)
	{
        if (!IsFormatSupported(filename)) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "zipextractor: unsupported archive format");
			return false;
		}

		CurFile.open(filename, std::ios::binary);

		if (!CurFile.is_open()) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "zipextractor: couldn't open the archive");
			return false;
		}

		CurFileName = filename;

		uint16_t sig;
		CurFile.seekg(0);
		CurFile.read(reinterpret_cast<char*>(&sig), sizeof(sig));

#ifdef __BIG_ENDIAN__
		sig = byteswap(sig);
#endif
		CurFile.seekg(0);

        if (sig == 0x8b1f)
            while(scanGZipHeader()) {}
        else
            while(scanZipHeader()) {}

		std::sort(ZipEntries.begin(), ZipEntries.end());

		for (const auto &entry : ZipEntries) {
			createFileForEntry(entry, destination);
		}

		return true;
	}

	bool ZipExtractor::scanGZipHeader()
	{
		ZipFileEntry entry;
		entry.Offset = 0;
		memset(&entry.Header, 0, sizeof(ZIPFileHeader));

		// read header
		GZIPMemberHeader header;
        CurFile.read(reinterpret_cast<char*>(&header), sizeof(GZIPMemberHeader));
        if (CurFile.tellg() != sizeof(GZIPMemberHeader))
			return false;

#ifdef __BIG_ENDIAN__
		header.sig = byteswap(header.sig);
		header.time = byteswap(header.time);
#endif

		// check header value
		if (header.sig != 0x8b1f)
			return false;

		// now get the file info
		if (header.flags & GZF_EXTRA_FIELDS) {
			// read lenth of extra data
			uint16_t dataLen;

			CurFile.read(reinterpret_cast<char*>(&dataLen), sizeof(dataLen));

#ifdef __BIG_ENDIAN__
			dataLen = byteswap(dataLen);
#endif

			// skip it
			CurFile.seekg(dataLen, std::ios_base::cur);
		}

		if (header.flags & GZF_FILE_NAME) {
			char c;
			CurFile.read(&c, sizeof(c));
			while (c) {
				entry.Path += c;
				CurFile.read(&c, sizeof(c));
			}
		} else {
			// no file name?
			entry.Path = FILENAME_BS(CurFileName);
			std::string ext = EXTENSION_BS(CurFileName);

			// rename tgz to tar or remove gz extension
            if (ext == "tgz") {
				entry.Path[CurFileName.size() - 2] = 'a';
				entry.Path[CurFileName.size() - 1] = 'r';
			} else if (ext == "gz") {
                entry.Path.erase(entry.Path.end()-3);
			}
		}

        if (header.flags & GZF_COMMENT) {
			char c = 'a';
			while (c)
				CurFile.read(&c, sizeof(c));
		}

        if (header.flags & GZF_CRC16)
			CurFile.seekg(2, std::ios_base::cur);

		// we are now at the start of the data blocks
		entry.Offset = CurFile.tellg();

		entry.Header.FilenameLength = entry.Path.size();

		entry.Header.CompressionMethod = header.compressionMethod;
		entry.Header.DataDescriptor.CompressedSize = (getFileSize(CurFile) - 8) - entry.Offset;

		// seek to file end
		CurFile.seekg(entry.Header.DataDescriptor.CompressedSize, std::ios_base::cur);

		// read CRC
		CurFile.read(reinterpret_cast<char*>(&entry.Header.DataDescriptor.CRC32), sizeof(uint32_t));
		// read uncompressed size
		CurFile.read(reinterpret_cast<char*>(&entry.Header.DataDescriptor.UncompressedSize), sizeof(uint32_t));

#ifdef __BIG_ENDIAN__
		entry.Header.DataDescriptor.CRC32 = byteswap(entry.Header.DataDescriptor.CRC32);
		entry.Header.DataDescriptor.UncompressedSize = byteswap(entry.Header.DataDescriptor.UncompressedSize);
#endif

		std::replace(entry.Path.begin(), entry.Path.end(), '\\', '/');

        if (entry.Path.back() == '/') {
			entry.IsDirectory = true;
			entry.Path.erase(entry.Path.end()-1);
		}

		entry.Size = entry.Header.DataDescriptor.UncompressedSize;
		ZipEntries.push_back(entry);
		// there's only one block of data in a gzip file
		return true;
	}

	//! scans for a local header, returns false if there is no more local file header.
	bool ZipExtractor::scanZipHeader(bool ignoreGPBits)
	{
		ZipFileEntry entry;
		entry.Offset = 0;
        memset(&entry.Header, 0, sizeof(ZIPFileHeader));

		CurFile.read(reinterpret_cast<char*>(&entry.Header), sizeof(ZIPFileHeader));

#ifdef __BIG_ENDIAN__
		entry.Header.Sig = byteswap(entry.Header.Sig);
		entry.Header.VersionToExtract = byteswap(entry.Header.VersionToExtract);
		entry.Header.GeneralBitFlag = byteswap(entry.Header.GeneralBitFlag);
		entry.Header.CompressionMethod = byteswap(entry.Header.CompressionMethod);
		entry.Header.LastModFileTime = byteswap(entry.Header.LastModFileTime);
		entry.Header.LastModFileDate = byteswap(entry.Header.LastModFileDate);
		entry.Header.DataDescriptor.CRC32 = byteswap(entry.Header.DataDescriptor.CRC32);
		entry.Header.DataDescriptor.CompressedSize = byteswap(entry.Header.DataDescriptor.CompressedSize);
		entry.Header.DataDescriptor.UncompressedSize = byteswap(entry.Header.DataDescriptor.UncompressedSize);
		entry.Header.FilenameLength = byteswap(entry.Header.FilenameLength);
		entry.Header.ExtraFieldLength = byteswap(entry.Header.ExtraFieldLength);
#endif

		if (entry.Header.Sig != 0x04034b50)
			return false; // local file headers end here.

		// read filename
		{
			char *tmp = new char[entry.Header.FilenameLength + 2];
			CurFile.read(tmp, entry.Header.FilenameLength);
			tmp[entry.Header.FilenameLength] = 0;
			entry.Path = tmp;
			delete[] tmp;
		}

		if (entry.Header.ExtraFieldLength)
			CurFile.seekg(entry.Header.ExtraFieldLength, std::ios_base::cur);

		// if bit 3 was set, use CentralDirectory for setup
        if (ignoreGPBits && entry.Header.GeneralBitFlag & 0x0008) {
			ZIPFileCentralDirEnd dirEnd;
			ZipEntries.clear();
			// First place where the end record could be stored
			CurFile.seekg(getFileSize(CurFile) - 22);
			const char endID[] = {0x50, 0x4b, 0x05, 0x06, 0x0};
			char tmp[5] = {'\0'};
			bool found = false;
			// search for the end record ID
			while (!found && CurFile.tellg() > 0) {
				int seek = 8;
				CurFile.read(tmp, 4);
				switch (tmp[0]) {
				case 0x50:
					if (!strcmp(endID, tmp)) {
						seek = 4;
						found = true;
					}
					break;
				case 0x4b:
					seek = 5;
					break;
				case 0x05:
					seek = 6;
					break;
				case 0x06:
					seek = 7;
					break;
				}
				CurFile.seekg(-seek, std::ios_base::cur);
			}
			CurFile.read(reinterpret_cast<char*>(&dirEnd), sizeof(dirEnd));
#ifdef __BIG_ENDIAN__
			dirEnd.NumberDisk = byteswap(dirEnd.NumberDisk);
			dirEnd.NumberStart = byteswap(dirEnd.NumberStart);
			dirEnd.TotalDisk = byteswap(dirEnd.TotalDisk);
			dirEnd.TotalEntries = byteswap(dirEnd.TotalEntries);
			dirEnd.Size = byteswap(dirEnd.Size);
			dirEnd.Offset = byteswap(dirEnd.Offset);
			dirEnd.CommentLength = byteswap(dirEnd.CommentLength);
#endif
			ZipEntries.reserve(dirEnd.TotalEntries);
            CurFile.seekg(dirEnd.Offset);
			while (scanCentralDirectoryHeader()) {
			}
			return false;
		}

		// store position in file
		entry.Offset = CurFile.tellg();
		// move forward length of data
        CurFile.seekg(entry.Header.DataDescriptor.CompressedSize, std::ios_base::cur);

		std::replace(entry.Path.begin(), entry.Path.end(), '\\', '/');

        if (entry.Path.back() == '/') {
			entry.IsDirectory = true;
			entry.Path.erase(entry.Path.end()-1);
		}

		entry.Size = entry.Header.DataDescriptor.UncompressedSize;
		ZipEntries.push_back(entry);

		return true;
	}

	//! scans for a local header, returns false if there is no more local file header.
	bool ZipExtractor::scanCentralDirectoryHeader()
	{
		ZIPFileCentralDirFileHeader entry;
        CurFile.read(reinterpret_cast<char*>(&entry), sizeof(ZIPFileCentralDirFileHeader));

#ifdef __BIG_ENDIAN__
		entry.Sig = byteswap(entry.Sig);
		entry.VersionMadeBy = byteswap(entry.VersionMadeBy);
		entry.VersionToExtract = byteswap(entry.VersionToExtract);
		entry.GeneralBitFlag = byteswap(entry.GeneralBitFlag);
		entry.CompressionMethod = byteswap(entry.CompressionMethod);
		entry.LastModFileTime = byteswap(entry.LastModFileTime);
		entry.LastModFileDate = byteswap(entry.LastModFileDate);
		entry.CRC32 = byteswap(entry.CRC32);
		entry.CompressedSize = byteswap(entry.CompressedSize);
		entry.UncompressedSize = byteswap(entry.UncompressedSize);
		entry.FilenameLength = byteswap(entry.FilenameLength);
		entry.ExtraFieldLength = byteswap(entry.ExtraFieldLength);
		entry.FileCommentLength = byteswap(entry.FileCommentLength);
		entry.DiskNumberStart = byteswap(entry.DiskNumberStart);
		entry.InternalFileAttributes = byteswap(entry.InternalFileAttributes);
		entry.ExternalFileAttributes = byteswap(entry.ExternalFileAttributes);
		entry.RelativeOffsetOfLocalHeader = byteswap(entry.RelativeOffsetOfLocalHeader);
#endif

		if (entry.Sig != 0x02014b50)
			return false; // central dir headers end here.

        const long pos = CurFile.tellg();
        CurFile.seekg(entry.RelativeOffsetOfLocalHeader);
		scanZipHeader(true);
        CurFile.seekg(pos + entry.FilenameLength + entry.ExtraFieldLength + entry.FileCommentLength);
		auto &lastInfo = ZipEntries.back();
		lastInfo.Header.DataDescriptor.CompressedSize = entry.CompressedSize;
		lastInfo.Header.DataDescriptor.UncompressedSize = entry.UncompressedSize;
		lastInfo.Header.DataDescriptor.CRC32 = entry.CRC32;
		lastInfo.Size = entry.UncompressedSize;
		return true;
	}

	void ZipExtractor::createFileForEntry(const ZipFileEntry &entry, const std::string &destination)
	{
		if (entry.IsDirectory)
			return;

		fs::create_directories(fs::path(destination));

		std::ofstream outFile;
		uint32_t csize = entry.Header.DataDescriptor.CompressedSize;
		uint32_t ucsize = entry.Header.DataDescriptor.UncompressedSize;

        outFile.open(destination + entry.Path, std::ios_base::binary);
		switch (entry.Header.CompressionMethod) {
			case 0: {
				return;
			}
			case 8: {
                char *pBuf = new char[ucsize];
				if (!pBuf) {
					std::string errLog = "zipextractor: not enough memory for decompressing " + entry.Path;
					SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, errLog.c_str());
					delete[] pBuf;
					return;
				}

				uint8_t *pcData = new uint8_t[csize];
				if (!pcData) {
					std::string errLog = "zipextractor: not enough memory for decompressing " + entry.Path;
					SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, errLog.c_str());
					delete[] pBuf;
					delete[] pcData;
					return;
				}

				CurFile.seekg(entry.Offset);
				CurFile.read(reinterpret_cast<char*>(pcData), csize);

				// Setup the inflate stream.
				z_stream stream;
				int32_t err;

				stream.next_in = (Bytef *)pcData;
				stream.avail_in = (uInt)csize;
				stream.next_out = (Bytef *)pBuf;
				stream.avail_out = ucsize;
				stream.zalloc = (alloc_func)0;
				stream.zfree = (free_func)0;

				// Perform inflation. wbits < 0 indicates no zlib header inside the data.
				err = inflateInit2(&stream, -MAX_WBITS);
				if (err == Z_OK) {
					err = inflate(&stream, Z_FINISH);
					inflateEnd(&stream);
					if (err == Z_STREAM_END)
						err = Z_OK;
					err = Z_OK;
					inflateEnd(&stream);
				}

				delete[] pcData;
				if (err != Z_OK) {
					std::string errLog = "zipextractor: error when decompressing " + entry.Path;
					SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, errLog.c_str());
					delete[] pBuf;
				}
				else {
					outFile << pBuf;
				}

				return;
			}
			case 12: {
				std::string errLog = "zipextractor: bzip2 decompression not supported. File cannot be read.";
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, errLog.c_str());
				return;
			}
			case 14: {
				std::string errLog = "zipextractor: lzma decompression not supported. File cannot be read.";
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, errLog.c_str());
				return;
			}
			case 99:{
				// If we come here with an encrypted file, decryption support is missing
				std::string errLog = "zipextractor: decryption support not enabled. File cannot be read.";
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, errLog.c_str());
				return;
			}
			default: {
				std::string errLog = "zipextractor: file has unsupported compression method. " + entry.Path;
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, errLog.c_str());
				return;
			}
		};
	}
};
