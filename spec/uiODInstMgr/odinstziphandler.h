#ifndef odinstziphandler_h
#define odinstziphandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          Feb 2012
 RCS:           $Id: odinstziphandler.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "bufstring.h"

class BufferStringSet;
class FilePath;
class TaskRunner;


namespace ODInst
{

class AppData;
class PkgProps;

class ZipHandler
{
public:
			ZipHandler(const BufferStringSet& zipfiles,
			    const AppData&);
			~ZipHandler();
    bool		installZipPackages(TaskRunner*);
    void		makeFileList() const;
    BufferString	errMsg() const { return errmsg_; }    
protected:

    const AppData&	appdata_;
    const BufferStringSet&  zipfiles_;
    FilePath&		filelistfp_;
    mutable BufferString    errmsg_;
};

} // namesapce ODInst

#endif

