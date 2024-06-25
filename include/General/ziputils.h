#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "bufstringset.h"
#include "executor.h"
#include "ziphandler.h"


/*!
\brief Zip Utilities
  For zipping, the file names that go into the archive are relative to the
  provided basepath. If the input path is a directory, all files inside
  the directory are recursively going into the archive.
  For unzipping, the basepath is prepended to the relative file paths in the
  archive, to determine the output file location.
  basepath may be set to null, in which case it will be determined by
  base filepath of the input/output archive filename.
*/

mExpClass(General) ZipUtils
{ mODTextTranslationClass(zipUtils);
public:

    static const char*		getZLibVersion();

    static bool			makeZip(const char* fileordirinp,
					const char* basepath,
					const char* zipfilenm,
					uiString& errmsg,
					TaskRunner* =nullptr,
					ZipHandler::CompLevel =
					ZipHandler::Normal);
    static bool			makeZip(const BufferStringSet& fullsrcfnms,
					const char* basepath,
					const char* zipfilenm,
					uiString& errmsg,
					TaskRunner* =nullptr,
					ZipHandler::CompLevel =
					ZipHandler::Normal);
    static bool			appendToArchive(const char* fileordirinp,
						const char* basepath,
						const char* zipfilenm,
						uiString& errmsg,
						TaskRunner* =nullptr,
						ZipHandler::CompLevel =
						ZipHandler::Normal);

    static bool			unZipArchive(const char* zipfilenm,
					     const char* dest,uiString& errmsg,
					     TaskRunner* =nullptr);
    static bool			unZipFile(const char* zipfilenm,
					  const char* srcfnm,const char* dest,
					  uiString& errmsg,
					  TaskRunner* =nullptr);
    static bool			unZipArchives(const BufferStringSet& zipfilenms,
					      const char* dest,uiString& errmsg,
					      TaskRunner* =nullptr);

    static bool			makeFileList(const char* zipfilenm,
					     BufferStringSet& list,
					     uiString& errmsg);

};
