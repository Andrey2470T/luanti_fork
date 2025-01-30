#include <string>
#include <vector>
#include <cctype>
#include <fstream>
#include "zipStructs.h"
#include "byteswap.h"


namespace extractor
{
	class ZipExtractor {
		//! Contains extended info about zip files in the archive
		struct ZipFileEntry
		{
            std::string Path = "";
			uint32_t Size;
			bool IsDirectory;
			//! Position of data in the archive file
			int32_t Offset;

			//! The header for this file containing compression info etc
			ZIPFileHeader Header;

            std::string ignore_case(const std::string &str) const
            {
                std::string lower_str = str;
                for (char &ch : lower_str)
                    ch = std::tolower(ch);

                return lower_str;
            }

			bool operator==(const ZipFileEntry &other) const
			{
				if (IsDirectory != other.IsDirectory)
					return false;

                return ignore_case(Path) == ignore_case(other.Path);
			}

			//! The < operator is provided so that CFileList can sort and quickly search the list.
			bool operator<(const ZipFileEntry &other) const
			{
				if (IsDirectory != other.IsDirectory)
					return IsDirectory;

				return ignore_case(Path) < ignore_case(other.Path);
			}
		};

		std::ifstream CurFile;
		std::string CurFileName = "";
		std::vector<ZipFileEntry> ZipEntries;
	public:
		ZipExtractor() = default;
		static bool IsFormatSupported(const std::string &filename);
		bool extractArchive(const std::string &filename, const std::string &destination);
	private:
		bool scanGZipHeader();
        bool scanZipHeader(bool ignoreGPBits = false);
		bool scanCentralDirectoryHeader();
		void createFileForEntry(const ZipFileEntry &entry, const std::string &destination);
		std::string &ignore_case(const std::string &str);
	};
};
