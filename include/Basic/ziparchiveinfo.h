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

mClass(Basic) ZipArchiveInfo
{
public:

    mClass(Basic) FileInfo
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
	unsigned int		compsize_;
	unsigned int		uncompsize_;
	unsigned int		localheaderoffset_;
    };

				ZipArchiveInfo( BufferString& fnm );
				~ZipArchiveInfo();

    bool			isOK(){ return isok_; }
    const char*			errorMsg();

    bool			getAllFnms( BufferStringSet& );

    				//!< All sizes in Bytes
    od_int64			getFileCompSize( BufferString& fnm );
    od_int64			getFileCompSize( int );
    od_int64			getFileUnCompSize( BufferString& fnm );
    od_int64			getFileUnCompSize( int );

    od_int64			getLocalHeaderOffset( BufferString& fnm );
    od_int64			getLocalHeaderOffset( int );

protected:

    bool			readZipArchive( BufferString& fnm );
    ObjectSet<FileInfo>		files_;
    ZipHandler&			ziphd_;
    BufferString		errormsg_;
    bool			isok_;

};



#endif
