/*
 *  Part of The TCMP Matroska CDL, and Matroska Shell Extension
 *
 *  ZipArchive.cpp
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
    \file ZipArchive.cpp
		\version \$Id$
    \brief A lightwight Zip Archive class
		\author Jory Stone     <jcsston @ toughguy.net>
*/

#include "ZipArchive.h"

ZipArchiveWriter::ZipArchiveWriter(const wchar_t *outputArchive) {
	long sourceStrLen = wcslen(outputArchive)+1;
	m_OutputFilename.resize(sourceStrLen);
	for (size_t cw1 = 0; cw1 < sourceStrLen; cw1++) 
		m_OutputFilename[cw1] = outputArchive[cw1];	

	m_bClosed = false;
	m_OutputFile = fopen(m_OutputFilename.c_str(), "wb");
};

ZipArchiveWriter::ZipArchiveWriter(const std::string &outputArchive) {
	m_OutputFilename = outputArchive;
	m_bClosed = false;
	m_OutputFile = fopen(m_OutputFilename.c_str(), "wb");
};

ZipArchiveWriter::~ZipArchiveWriter() {
	if (!m_bClosed)
		Close();
};

bool ZipArchiveWriter::IsOk() {
	if (!m_bClosed && m_OutputFile != NULL)
		return true;

	return false;
};

void ZipArchiveWriter::AddFile(const wchar_t *sourceFilename) {
	std::string sourceStr;
	long sourceStrLen = wcslen(sourceFilename)+1;
	sourceStr.resize(sourceStrLen);
	for (size_t cw1 = 0; cw1 < sourceStrLen; cw1++) 
		sourceStr[cw1] = sourceFilename[cw1];	

	AddFile(sourceStr);
}

void ZipArchiveWriter::AddFile(const std::string &sourceFilename) {
	FILE *inputFile = fopen(sourceFilename.c_str(), "rb");
#ifdef _DEBUG
	m_ZipComment += "Opening :" + sourceFilename;
#endif	
	if (inputFile != NULL) {
#ifdef _DEBUG
		m_ZipComment += " - Opened";
#endif
		fseek(inputFile, 0, SEEK_END);
		DWORD fileLength = ftell(inputFile);
		fseek(inputFile, 0, SEEK_SET);
		// 64 MB is the limit to read in
		if (fileLength < 64 * 1024 * 1024) {
			BYTE *buffer = new BYTE[fileLength];
			fread(buffer, 1, fileLength, inputFile);
#ifdef _DEBUG
			m_ZipComment += "Read the data";
#endif
			std::string filenameOnly = sourceFilename.substr(1+sourceFilename.find_last_of("\\"));
			if (filenameOnly.empty()) {
				// No \'s ?
				filenameOnly = sourceFilename.substr(1+sourceFilename.find_last_of("/"));
				if (filenameOnly.empty()) {
					// No /'s ?
					filenameOnly = sourceFilename;
				}
			}
#ifdef _DEBUG
			m_ZipComment += "Using plain filename: " + filenameOnly;
#endif
			AddFile(filenameOnly, buffer, fileLength);
			delete buffer;
		}
#ifdef _DEBUG
		else
			m_ZipComment += " File too large to load into memory.";
#endif
		fclose(inputFile);
	}
};

void ZipArchiveWriter::AddFile(const std::string &newFile, const unsigned char *data, unsigned long size) {
#ifdef _DEBUG
	m_ZipComment += "Adding File: " + newFile + " from memory";
#endif
	//while (true);
	// Start creating this file entry footer
	CZip_central_dir_header fileFooter;
	fileFooter.data.relative_offset_of_local_header = ftell(m_OutputFile);

	// Setup the header for this file entry
	zip_local_file_header fileHeader;
	memset(&fileHeader, 0, sizeof(zip_local_file_header));
	fileHeader.header_sig = 0x04034b50;
	fileHeader.version = 10; // 1.0 ?

	DosDate ddDate;
	DosTime dtTime;
	time_t currentTime = time(NULL);
	tm *currentTm = gmtime(&currentTime);	
	ddDate.nMonthDay = currentTm->tm_mday;
	ddDate.nMonth = currentTm->tm_mon + 1;
	ddDate.nYear = currentTm->tm_year + 1900 - 1980;
	
	dtTime.nSecond = currentTm->tm_sec / 2;
	dtTime.nHour = currentTm->tm_hour;
	dtTime.nMinute = currentTm->tm_min;

	memcpy(&fileHeader.last_mod_file_date, &ddDate, sizeof(WORD));
	memcpy(&fileHeader.last_mod_file_time, &dtTime, sizeof(WORD));

  fileHeader.compression_method = Z_DEFLATED; // Good ole Deflate
  
  // Compress the data
  z_stream zlibStream;
  memset(&zlibStream, 0, sizeof(z_stream));
  
  // Setup the zlib stream with Max compression
  int ret = deflateInit(&zlibStream, Z_BEST_COMPRESSION);
  if (ret == Z_OK) {
    // zlib Stream setup correctly
    
    // Setup the input data buffer
    zlibStream.next_in = (Byte *)data;
    zlibStream.avail_in = size;

    // Setup the output data buffer
    // Even in the worst case I doubt the compressed data would take 25% more than the orignal
    uLong compressedBufferSize = (float)size * 1.25;
    Byte *compressedBuffer = new Byte[compressedBufferSize];

    zlibStream.next_out = (Byte *)compressedBuffer;
    zlibStream.avail_out = compressedBufferSize;

    ret = deflate(&zlibStream, Z_FINISH);
    if (ret == Z_STREAM_END) {
			// I take 2 bytes off for the zlib? header
  	 fileHeader.compressed_size = zlibStream.total_out-2;
    } else if (ret == Z_OK) {
      // Not enough output space
      
      // Make the buffer larger
      delete compressedBuffer;
      compressedBufferSize = (float)compressedBufferSize * 1.5;
      compressedBuffer = new Byte[compressedBufferSize];

      // Update the stream pointers/sizes
      zlibStream.next_out = (Byte *)compressedBuffer;
      zlibStream.avail_out = compressedBufferSize;
      
      ret = deflate(&zlibStream, Z_FINISH);
      if (ret == Z_STREAM_END) {
				// I take 2 bytes off for the zlib? header
        fileHeader.compressed_size = zlibStream.total_out-2;
      }
		}
    fileHeader.uncompressed_size = size;
		
		// Create the CRC32
		fileHeader.crc32 = crc32(0L, Z_NULL, 0);
		fileHeader.crc32 = crc32(fileHeader.crc32, data, size);
    
  	fileHeader.file_name_length = newFile.length();

  	// Write the header + filename
  	fwrite(&fileHeader, 1, sizeof(zip_local_file_header), m_OutputFile);
  	fwrite(newFile.c_str(), 1, fileHeader.file_name_length, m_OutputFile);
  	// Now the data
		// The 2 byte offset is for the zlib? header
  	fwrite(compressedBuffer+2, 1, fileHeader.compressed_size, m_OutputFile);

    // We wrote the data, no need to keep it in memory
    delete compressedBuffer;
    compressedBuffer = NULL;
    
  	// Finish creating this file entry footer
  	fileFooter.data.version_needed = fileHeader.version;
  	fileFooter.data.uncompressed_size = fileHeader.uncompressed_size;
  	fileFooter.data.compressed_size = fileHeader.compressed_size;

  	fileFooter.data.general_flags = fileHeader.general_flags;

		fileFooter.data.last_mod_file_date = fileHeader.last_mod_file_date;
		fileFooter.data.last_mod_file_time = fileHeader.last_mod_file_time;

  	fileFooter.file_name = newFile;
  	fileFooter.data.file_name_length =  fileHeader.file_name_length;

  	fileFooter.data.crc32 = fileHeader.crc32;

  	// Push this file entry footer into the vector,
  	// we don't write them until the end of the file
  	m_FileList.push_back(fileFooter);
	}
};

void ZipArchiveWriter::Close() {
	zip_central_dir_footer fileFooter;
	memset(&fileFooter, 0, sizeof(zip_central_dir_footer));
	fileFooter.header_sig = 0x06054b50;
	fileFooter.dir_offset = ftell(m_OutputFile);
	fileFooter.entry_count_disk_total = m_FileList.size();
	fileFooter.entry_count_total = m_FileList.size();
	for (size_t z = 0; z < m_FileList.size(); z++) {
		CZip_central_dir_header &entryFooter = m_FileList.at(z);
		entryFooter.Write(m_OutputFile);
	}
	
	fileFooter.size_central_dir = ftell(m_OutputFile) - fileFooter.dir_offset;
	

	fileFooter.file_comment_length = m_ZipComment.length();
	
	fwrite(&fileFooter, 1, sizeof(zip_central_dir_footer), m_OutputFile);
	fwrite(m_ZipComment.c_str(), 1, fileFooter.file_comment_length, m_OutputFile);
		
	m_bClosed = true;
	fclose(m_OutputFile);
	m_OutputFile = NULL;
};
