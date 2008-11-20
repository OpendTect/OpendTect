/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segydirect.cc,v 1.5 2008-11-20 13:26:15 cvsbert Exp $";

#include "segydirectdef.h"
#include "segyfiledata.h"
#include "strmprov.h"
#include "ascstream.h"
#include "keystrs.h"

const char* sKeyDirectDef = "DirectSEG-Y";
static const char* sKeyFileType = "SEG-Y Direct Definition";
static const char* sKeyNrFiles = "Number of files";


SEGY::DirectDef::DirectDef( Seis::GeomType gt )
    : geom_(gt)
    , fds_(0)
    , myfds_(0)
    , curfidx_(-1)
{
}


SEGY::DirectDef::DirectDef( const char* fnm )
    : geom_(Seis::VolPS)
    , fds_(0)
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


bool SEGY::DirectDef::readFromFile( const char* fnm )
{
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	return false;

    ascistream astrm( *sd.istrm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	return false;

    int nrfiles = 0;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Geometry) )
	    geom_ = Seis::geomTypeOf( astrm.value() );
	else if ( astrm.hasKeyword(sKeyNrFiles) )
	    nrfiles = atoi( astrm.value() );
    }

    IOPar iop; iop.getFrom( astrm );
    delete myfds_; myfds_ = new FileDataSet(iop); fds_ = myfds_;

    for ( int idx=0; idx<nrfiles; idx++ )
    {
	FileData* fd = new FileData(0);
	fd->getFrom( astrm, geom_ );
    }

    return true;
}


bool SEGY::DirectDef::writeToFile( const char* fnm ) const
{
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	return false;

    const int nrfiles = fds_ ? fds_->size() : 0;
    ascostream astrm( *sd.ostrm );
    astrm.putHeader( sKeyFileType );
    astrm.put( sKey::Geometry, Seis::nameOf(geom_) );
    astrm.put( sKeyNrFiles, nrfiles );
    astrm.newParagraph();
    if ( fds_ )
	fds_->pars().putTo( astrm );

    for ( int ifile=0; ifile<nrfiles; ifile++ )
    {
	const SEGY::FileData& fd = *(*fds_)[ifile];
	if ( !fd.putTo(astrm,geom_) )
	    return false;
    }

    return true;
}
