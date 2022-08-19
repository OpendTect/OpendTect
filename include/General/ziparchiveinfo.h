#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "objectset.h"
#include "bufstring.h"

class BufferStringSet;
class ZipFileInfo;


/*!
\brief Gives information of zip archives.
*/

mExpClass(General) ZipArchiveInfo
{
public:

				ZipArchiveInfo(const char* fnm);
				~ZipArchiveInfo();

    bool			isOK() const { return isok_; }
    const char*			errorMsg() const;

    bool			getAllFnms(BufferStringSet&) const;
    od_int64			getTotalSize(bool uncomp=true) const;

    //!< All sizes in Bytes
    od_int64			getFileCompSize(const char* fnm) const;
    od_int64			getFileCompSize(int) const;
    od_int64			getFileUnCompSize(const char* fnm) const;
    od_int64			getFileUnCompSize(int) const;

    od_int64			getLocalHeaderOffset(const char* fnm) const;
    od_int64			getLocalHeaderOffset(int) const;

protected:

    bool			readZipArchive(const char* fnm);

    ObjectSet<ZipFileInfo>	fileinfo_;
    mutable BufferString	errormsg_;
    bool			isok_;

};
