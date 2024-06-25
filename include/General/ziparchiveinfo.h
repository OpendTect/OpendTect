#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "objectset.h"
#include "uistring.h"

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
    uiString			errMsg() const;

    bool			getAllFnms(BufferStringSet&) const;
    od_int64			getTotalSize(bool uncomp=true) const;

    //!< All sizes in Bytes
    od_int64			getFileCompSize(const char* fnm) const;
    od_int64			getFileCompSize(int) const;
    od_int64			getFileUnCompSize(const char* fnm) const;
    od_int64			getFileUnCompSize(int) const;

    bool			get(const char* fnm,ZipFileInfo&) const;
    const ZipFileInfo*		getInfo(const char* fnm) const;

private:

    bool			readZipArchive();
    void			setFileNotPresentError(const char* fnm);

    BufferString		srcfnm_;
    ObjectSet<ZipFileInfo>	fileinfo_;
    mutable uiString		errormsg_;
    bool			isok_;

};
