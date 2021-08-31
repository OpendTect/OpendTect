#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:	Salil Agarwal
Date:		27 August 2012
RCS:		$Id$
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


