#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:	Salil Agarwal
Date:		27 August 2012
________________________________________________________________________

-*/

#include "generalmod.h"
#include "objectset.h"
#include "bufstring.h"
#include "uistrings.h"

class BufferStringSet;
class ZipFileInfo;


/*!
\brief Gives information of zip archives.
*/

mExpClass(General) ZipArchiveInfo
{ mODTextTranslationClass(ZipArchiveInfo)
public:

				ZipArchiveInfo(const char* fnm);
				~ZipArchiveInfo();

    bool			isOK() const { return isok_; }
    const uiString		errorMsg() const;

    bool			getAllFnms(BufferStringSet&) const;

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
    mutable uiString		errormsg_;
    bool			isok_;

};
