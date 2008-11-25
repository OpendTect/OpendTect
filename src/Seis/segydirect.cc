/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segydirect.cc,v 1.6 2008-11-25 11:37:46 cvsbert Exp $";

#include "segydirectdef.h"
#include "segyfiledata.h"
#include "strmprov.h"
#include "ascstream.h"
#include "keystrs.h"

const char* sKeyDirectDef = "DirectSEG-Y";
static const char* sKeyFileType = "SEG-Y Direct Definition";
static const char* sKeyNrFiles = "Number of files";


SEGY::DirectDef::DirectDef()
    : fds_(0)
    , myfds_(0)
    , curfidx_(-1)
{
}


SEGY::DirectDef::DirectDef( const char* fnm )
    : fds_(0)
    , myfds_(0)
    , curfidx_(-1)
{
    readFromFile( fnm );
}


SEGY::DirectDef::~DirectDef()
{
    delete myfds_;
}


void SEGY::DirectDef::setData( FileDataSet* fds )
{
    if ( fds != myfds_ )
	delete myfds_;
    fds_ = myfds_ = fds;
}


void SEGY::DirectDef::setData( const FileDataSet& fds, bool nc )
{
    if ( &fds != myfds_ )
	delete myfds_;
    if ( nc )
	{ myfds_ = 0; fds_ = &fds; }
    else
	fds_ = myfds_ = new FileDataSet( fds );
}


#define mErrRet(s) { errmsg_ = s; return false; }

bool SEGY::DirectDef::readFromFile( const char* fnm )
{
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet(BufferString("Cannot open '",fnm,"'"))

    ascistream astrm( *sd.istrm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	mErrRet(BufferString("Input file '",fnm,"' has wrong file type"))

    int nrfiles = 0;
    IOPar iop; iop.getFrom( astrm );
    iop.get( sKeyNrFiles, nrfiles );
    Seis::GeomType gt;
    if ( !Seis::getFromPar(iop,gt) )
	mErrRet(BufferString("Missing crucial info in '",fnm,"'"))
    
    delete myfds_; myfds_ = new FileDataSet(iop); fds_ = myfds_;

    for ( int idx=0; idx<nrfiles; idx++ )
    {
	FileData* fd = new FileData(0,gt);
	if ( !fd->getFrom(astrm) )
	{
	    BufferString emsg( "Error reading " );
	    if ( nrfiles > 1 )
		{ emsg += idx+1; emsg += getRankPostFix(idx+1); emsg += " "; }
	    emsg += "file data from '"; emsg += fnm; emsg += "'";
	    mErrRet(emsg)
	}
    }

    return true;
}


bool SEGY::DirectDef::writeToFile( const char* fnm ) const
{
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	mErrRet(BufferString("Cannot open '",fnm,"' for write"))

    const int nrfiles = fds_ ? fds_->size() : 0;
    ascostream astrm( *sd.ostrm );
    astrm.putHeader( sKeyFileType );
    if ( fds_ )
    {
	IOPar iop( fds_->pars() );
	iop.set( sKeyNrFiles, nrfiles );
	iop.putTo( astrm );
    }

    for ( int ifile=0; ifile<nrfiles; ifile++ )
    {
	const SEGY::FileData& fd = *(*fds_)[ifile];
	if ( !fd.putTo(astrm) )
	{
	    BufferString emsg( "Error writing data for '" );
	    emsg += fd.fname_; emsg += "'"; emsg += "\nto '";
	    emsg += fnm; emsg += "'";
	    mErrRet(emsg)
	}
    }

    return true;
}
