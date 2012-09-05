/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Salil Agarwal
 Date:          27 August 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: ziparchiveinfo.cc,v 1.5 2012-09-05 03:14:31 cvssalil Exp $";

#include "ziparchiveinfo.h"

#include "bufstringset.h"
#include "ziphandler.h"
#include "strmprov.h"

#include <iostream>

/*
#include "file.h"
#include "filepath.h"
#include "dirlist.h"
#include "executor.h"
#include "task.h"
#include "iostream"
#include "fstream"
#include "utime.h"
*/

ZipArchiveInfo::ZipArchiveInfo( BufferString& fnm )
    : ziphd_(*new ZipHandler)
{
    readZipArchive( fnm );
}


ZipArchiveInfo::~ZipArchiveInfo()
{
    delete &ziphd_;
}


void ZipArchiveInfo::readZipArchive( BufferString& fnm )
{
    BufferString headerbuff, headerfnm;
    bool sigcheck;
    unsigned int ptrlocation;
    StreamData isd = StreamProvider( fnm ).makeIStream();
    if ( !isd.usable() )
	return;

    std::istream& src = *isd.istrm;
    ziphd_.readEndOfCntrlDirHeader( src );
    src.seekg( ziphd_.getOffsetOfCentralDir(), std::ios::beg );
    ptrlocation = src.tellg();
    for ( int i = 0; i < ziphd_.getTotalFiles(); i++ )
    {
	src.read( headerbuff.buf(), 46);
	mCntrlFileHeaderSigCheck( headerbuff, 0 );
	if ( !sigcheck )
	    return;
	src.seekg( ptrlocation + 46 );
	src.read( headerfnm.buf(), *( (short*) (headerbuff.buf() + 28) ) );
	FileInfo* fi = new FileInfo( headerfnm, 
				*( (unsigned int*) (headerbuff.buf() + 22) ),
				*( (unsigned int*) (headerbuff.buf() + 26) ),
				*( (unsigned int*) (headerbuff.buf() + 42) ) );
	files_ += fi;
	ptrlocation = ptrlocation + *( (short*) (headerbuff.buf() + 28) )
				  + *( (short*) (headerbuff.buf() + 30) )
				  + *( (short*) (headerbuff.buf() + 32) )
				  + 46;
	src.seekg( ptrlocation );
    }
    
    isd.close();

}

void ZipArchiveInfo::getAllFnms( BufferStringSet& fnms )
{
    for( int i = 0; i < ziphd_.getTotalFiles(); i++ )
	fnms.add( files_[i]->fnm_ );
}

unsigned int ZipArchiveInfo::getFCompSize( BufferString& fnm )
{
    for( int i = 0; i < ziphd_.getTotalFiles(); i++ )
	if ( fnm.matches( files_[i]->fnm_ ) )
	    return files_[i]->compsize_;
    return 0;
}

unsigned int ZipArchiveInfo::getFCompSize( int idx )
{
	    return files_[idx]->compsize_;
}

unsigned int ZipArchiveInfo::getFUnCompSize( BufferString& fnm )
{
    for( int i = 0; i < ziphd_.getTotalFiles(); i++ )
	if ( fnm.matches( files_[i]->fnm_ ) )
	    return files_[i]->uncompsize_;
    return 0;
}

unsigned int ZipArchiveInfo::getFUnCompSize( int idx )
{
    return files_[idx]->uncompsize_;
}

unsigned int ZipArchiveInfo::getLocalHeaderOffset( BufferString& fnm )
{
    fnm.buf()[fnm.size()] = '\0';
    for( int i = 0; i < ziphd_.getTotalFiles(); i++ )
    {
	/*std::cout<<files_[i]->localheaderoffset_<<"\n";*/
	files_[i]->fnm_.buf()[files_[i]->fnm_.size()] = '\0';
	if ( strstr( files_[i]->fnm_.buf(), fnm.buf() ) )
	    return files_[i]->localheaderoffset_;
    }
    return 0;
}

unsigned int ZipArchiveInfo::getLocalHeaderOffset( int idx )
{
    return files_[idx]->localheaderoffset_;
}
