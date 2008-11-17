/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segydirect.cc,v 1.3 2008-11-17 15:50:12 cvsbert Exp $";

#include "segydirectdef.h"
#include "segyfiledata.h"
#include "strmprov.h"


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
    return false;
}


bool SEGY::DirectDef::writeToFile( const char* fnm ) const
{
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	return false;
    return false;
}
