#ifndef ziparchiveinfo_h
#define ziparchiveinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		27 August 2012
 RCS:		$Id: ziparchiveinfo.h,v 1.4 2012-09-06 08:51:14 cvssalil Exp $
________________________________________________________________________

-*/

#include "objectset.h"
#include "basicmod.h"

class BufferStringSet;
class ZipHandler;

mClass(Basic) ZipArchiveInfo
{
public:

    class FileInfo
    {
    public:
				FileInfo( BufferString& fnm, 
					       unsigned int compsize, 
					       unsigned int uncompsize,
					       unsigned int offset )
					:fnm_(fnm)
					, compsize_(compsize) 
					, uncompsize_(uncompsize)
					, localheaderoffset_(offset)	{}
	BufferString		fnm_;
	unsigned int		compsize_, uncompsize_, localheaderoffset_;
    };

				ZipArchiveInfo( BufferString& fnm );
				~ZipArchiveInfo();

    bool			getAllFnms( BufferStringSet& );
    od_int64			getFCompSize( BufferString& fnm );
    od_int64			getFCompSize( int );
    od_int64			getFUnCompSize( BufferString& fnm );
    od_int64			getFUnCompSize( int );
    od_int64			getLocalHeaderOffset( BufferString& fnm );
    od_int64			getLocalHeaderOffset( int );
    const char*			errorMsg();
    bool			isOK(){ return isok_; }

protected:

    bool			readZipArchive( BufferString& fnm );
    ObjectSet<FileInfo>		files_;
    ZipHandler&			ziphd_;
    BufferString		errormsg_;
    bool			isok_;

};



#endif
