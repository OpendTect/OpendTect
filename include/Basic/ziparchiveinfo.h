#ifndef ziparchiveinfo_h
#define ziparchiveinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		27 August 2012
 RCS:		$Id: ziparchiveinfo.h,v 1.1 2012-08-31 05:32:54 cvssalil Exp $
________________________________________________________________________

-*/

#include "filepath.h"
#include "strmprov.h"
#include "ziputils.h"
#include "zlib.h"
#include "ziphandler.h"

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
    unsigned int		compsize_, uncompsize_, localheaderoffset_;
};

mClass(Basic) ZipArchiveInfo
{
public:
				ZipArchiveInfo( BufferString& fnm );
    void			getAllFnms( BufferStringSet& );
    unsigned int		getFCompSize( BufferString& fnm );
    unsigned int		getFCompSize( int );
    unsigned int		getFUnCompSize( BufferString& fnm );
    unsigned int		getFUnCompSize( int );
    unsigned int		getLocalHeaderOffset( BufferString& fnm );
    unsigned int		getLocalHeaderOffset( int );

protected:
    void			readZipArchive( BufferString& fnm );
    ObjectSet<FileInfo>		files_;
    ZipHandler			ziphd_;
};



#endif
