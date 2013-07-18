/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: odinstziphandler.cc 7944 2013-06-04 05:14:24Z ranojay.sen@dgbes.com $";

#include "odinstziphandler.h"

#include "file.h"
#include "filepath.h"
#include "odinstpkgprops.h"
#include "odinstappdata.h"
#include "strmprov.h"
#include "ziparchiveinfo.h"
#include "ziputils.h"

#include <iostream>


ODInst::ZipHandler::ZipHandler( const BufferStringSet& zipfiles, 
				const AppData& appdata )
    : appdata_(appdata)
    , filelistfp_(*new FilePath(appdata_.baseDirName(),appdata_.dirName(),
		   "relinfo"))
    , zipfiles_(zipfiles)
{
}


ODInst::ZipHandler::~ZipHandler()
{
    delete &filelistfp_;
}


bool ODInst::ZipHandler::installZipPackages( TaskRunner* tr )
{
    return ZipUtils::unZipArchives(zipfiles_,appdata_.baseDirName(),errmsg_,tr); 
}


void ODInst::ZipHandler::makeFileList() const
{
    for ( int idy=0; idy<zipfiles_.size(); idy++ )
    {
	const FilePath zipfilefp( zipfiles_.get(idy) );
	const BufferString listfnm( zipfilefp.fileName(), ".list" );
	FilePath listfp( filelistfp_, listfnm );
	if ( !File::isWritable(appdata_.baseDirName()) )
	{
	    errmsg_ = ". Could not obtain write permission to ";
	    errmsg_ += appdata_.baseDirName();
	    return ;
	}

	ZipArchiveInfo zinfo( zipfilefp.fullPath() );
	BufferStringSet filelist;
	zinfo.getAllFnms( filelist );
	StreamData sd = StreamProvider( listfp.fullPath() ).makeOStream();
	if ( !sd.usable() )
	{
	    sd.close();
	    return;
	}

	for ( int idx=0; idx<filelist.size(); idx++ )
	    *sd.ostrm << filelist.get( idx ) << std::endl;
	sd.close();
    }
}

