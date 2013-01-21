#ifndef ziparchiveinfo_h
#define ziparchiveinfo_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:	Salil Agarwal
Date:		27 August 2012
RCS:		$Id$
________________________________________________________________________

-*/

#include "objectset.h"
#include "basicmod.h"

class BufferStringSet;
class ZipHandler;
class ZipFileInfo;


/*!
\brief Gives information of zip archives.
*/

mExpClass(Basic) ZipArchiveInfo
{
public:

				ZipArchiveInfo(const char* fnm);
				~ZipArchiveInfo();

    bool			isOK() const { return isok_; }
    const char*			errorMsg() const;

    bool			getAllFnms(BufferStringSet&)const;

    //!< All sizes in Bytes
    od_int64			getFileCompSize(const char* fnm) const;
    od_int64			getFileCompSize(od_int32) const;
    od_int64			getFileUnCompSize(const char* fnm)const;
    od_int64			getFileUnCompSize(od_int32)const;

    od_int64			getLocalHeaderOffset(const char* fnm)const;
    od_int64			getLocalHeaderOffset(od_int32)const;

protected:

    bool			readZipArchive(const char* fnm);
    ObjectSet<ZipFileInfo>	fileinfo_;
    ZipHandler&			ziphd_;
    mutable BufferString	errormsg_;
    bool			isok_;

};


#endif
