/*
 *  Part of The TCMP Matroska CDL, and Matroska Shell Extension
 *
 *  ZipArchive.h
 *
 *  Copyright (C) Jory Stone - 2003
 *
 *  This file may be distributed under the terms of the Q Public License
 *  as defined by Trolltech AS of Norway and appearing in the file
 *  copying.txt included in the packaging of this file.
 *
 *  This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 *  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*!
    \file ZipArchive.h
		\version \$Id$
    \brief A lightwight Zip Archive class
		\author Jory Stone     <jcsston @ toughguy.net>
*/

#ifndef _ZIP_ARCHIVE_H_
#define _ZIP_ARCHIVE_H_

#include <windows.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <string.h>
#include <time.h>
#include "zlib.h"

#pragma pack(push, 1)
struct zip_local_file_header
{
	DWORD header_sig;// 4bytes (0x04034b50)
	WORD version;// needed to extract 	2 bytes
	WORD general_flags;// purpose bit flag 	2 bytes
	WORD compression_method;// 	2 bytes
	WORD last_mod_file_time;// 	2 bytes
	WORD last_mod_file_date;// 	2 bytes
	DWORD crc32;//-32 	4 bytes
	DWORD compressed_size; // 	4 bytes
	DWORD uncompressed_size; // 	4 bytes
	WORD file_name_length; // 	2 bytes
	WORD extra_field_length; // 	2 bytes
	//char *file_name; // 	(variable size)
	//char *extra_field; // 	(variable size)
};

// Only used if bit 3 is set on zip_local_file_header::general_flags
struct zip_data_footer
{
	DWORD crc32;//-32  	4 bytes
	DWORD compressed_size; // 	4 bytes
	DWORD uncompressed_size; // 	4 bytes
};

struct zip_central_dir_header
{
	DWORD header_sig; //  	4 bytes (0x02014b50)
	WORD version_by; // 	2 bytes
	WORD version_needed; // to extract 	2 bytes
	WORD general_flags;// purpose bit flag 	2 bytes
	WORD compression_method;// 	2 bytes
	WORD last_mod_file_time;// 	2 bytes
	WORD last_mod_file_date;// 	2 bytes
	DWORD crc32;//-32 	4 bytes
	DWORD compressed_size; // 	4 bytes
	DWORD uncompressed_size; // 	4 bytes
	WORD file_name_length; // 	2 bytes
	WORD extra_field_length; // 	2 bytes
	WORD comment_length; // 	2 bytes
	WORD disk_number_start; // 	2 bytes
	WORD internal_file_attributes; // 	2 bytes
	DWORD external_file_attributes; // 	4 bytes
	DWORD relative_offset_of_local_header; // 	4 bytes
	//char *file_name; // 	(variable size)
	//char *extra_field; // 	(variable size)
	//char *file_comment; // 	(variable size)
};

/*Digital signature:
header signature	4 bytes (0x05054b50)
size of data	2 bytes
signature data 	(variable size)*/

struct zip_central_dir_footer
{
	DWORD header_sig; //  	4 bytes (0x06054b50)
	WORD disk_number; //number of this disk 	2 bytes
	WORD disk_number_dir; // 	2 bytes
	WORD entry_count_disk_total; //WORD total number of entries in the central directory on this disk 	2 bytes
	WORD entry_count_total; //; the central directory 	2 bytes
	DWORD size_central_dir;// 	4 bytes
	DWORD dir_offset; //ectory_with_respect_to_the_starting disk number;// 	4 bytes
	WORD file_comment_length;// 	2 bytes
	//char *file_comment;// 	(variable size)
};
#pragma pack(pop)

class CZip_local_file_header {
public:
	CZip_local_file_header() {
		memset(&data, 0, sizeof(zip_local_file_header)); 
		data.header_sig = 0x04034b50;
	};

	zip_local_file_header data;
	std::string file_name;
	std::string extra_field;
};


class CZip_central_dir_header {
public:
	CZip_central_dir_header() { 
		memset(&data, 0, sizeof(zip_central_dir_header));
		data.header_sig = 0x02014b50;
		data.version_by = 20;
	};
	void Write(FILE *targetFile) {
		fwrite(&data, 1, sizeof(zip_central_dir_header), targetFile);
		
		fwrite(file_name.c_str(), 1, file_name.length(), targetFile);
		fwrite(extra_field.c_str(), 1, extra_field.length(), targetFile);
		fwrite(file_comment.c_str(), 1, file_comment.length(), targetFile);
	}

	zip_central_dir_header data;
	std::string file_name;
	std::string extra_field;
	std::string file_comment;
};

class ZipArchiveWriter {
public:
	ZipArchiveWriter(const wchar_t *outputArchive);
	ZipArchiveWriter(const std::string &outputArchive);
	~ZipArchiveWriter();

	bool IsOk();
	void SetComment(const std::string &newComment) { m_ZipComment = newComment; };	
	void AddFile(const wchar_t *sourceFilename);
	void AddFile(const std::string &sourceFilename);
	void AddFile(const std::string &newFile, const unsigned char *data, unsigned long size);
	void Close();

protected:
	bool m_bClosed;
	FILE *m_OutputFile;
	std::string m_OutputFilename;
	std::string m_ZipComment;
	struct DosDate {
		WORD nMonthDay : 5,
		nMonth : 4,
		nYear : 7;
	};
	struct DosTime {
		WORD nSecond : 5,
		nMinute : 6,
		nHour : 5;
	};

	std::vector<CZip_central_dir_header> m_FileList;
};

#endif // _ZIP_ARCHIVE_H_
