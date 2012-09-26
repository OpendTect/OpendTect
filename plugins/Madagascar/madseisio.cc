/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "madseisio.h"
#include "seisselection.h"


ODMad::SeisSeqIO::SeisSeqIO( Seis::GeomType gt, bool fr )
    : fspec_(fr)
    , gt_(gt)
{
}


ODMad::SeisSeqIO::~SeisSeqIO()
{
    sd_.close();
}


bool ODMad::SeisSeqIO::fromPar( const IOPar& iop )
{
    Seis::getFromPar( iop, gt_ );

    const bool isopen = sd_.usable();
    sd_.close(); fspec_.set( "" );
    if ( !fspec_.usePar(iop) )
	{ setErrMsg( fspec_.errMsg() ); return false; }

    return !isopen || open();
}


void ODMad::SeisSeqIO::toPar( IOPar& iop ) const
{
    Seis::putInPar( gt_, iop );
    fspec_.fillPar( iop );
}


ODMad::SeisSeqInp::SeisSeqInp( Seis::GeomType gt )
    : ODMad::SeisSeqIO(gt,true)
{
}


ODMad::SeisSeqInp::SeisSeqInp( Seis::GeomType gt, const FileSpec& fs )
    : ODMad::SeisSeqIO(gt,true)
{
    fspec_ = fs;
    open();
}


ODMad::SeisSeqInp::~SeisSeqInp()
{
}


bool ODMad::SeisSeqInp::usePar( const IOPar& iop )
{
    return fromPar( iop );
}


void ODMad::SeisSeqInp::fillPar( IOPar& iop ) const
{
    Seis::SeqInp::fillPar( iop );
    toPar( iop );
}


bool ODMad::SeisSeqInp::get( SeisTrc& trc ) const
{
    if ( !sd_.usable() && !const_cast<SeisSeqInp*>(this)->open() )
	return false;

    errmsg_ = "TODO: Not implemented: read next trace";
    return false;
}



bool ODMad::SeisSeqInp::open()
{
    pErrMsg("TODO: open Madagascar input");
    return false;
}


void ODMad::SeisSeqInp::initClass()
{
    Seis::SeqInp::factory().addCreator( create, ODMad::sKeyMadagascar );
}


ODMad::SeisSeqOut::SeisSeqOut( Seis::GeomType gt )
    : ODMad::SeisSeqIO(gt,false)
{
}


ODMad::SeisSeqOut::SeisSeqOut( Seis::GeomType gt, const FileSpec& fs )
    : ODMad::SeisSeqIO(gt,false)
{
    fspec_ = fs;
    open();
}


ODMad::SeisSeqOut::~SeisSeqOut()
{
}


void ODMad::SeisSeqOut::fillPar( IOPar& iop ) const
{
    Seis::SeqOut::fillPar( iop );
    toPar( iop );
}


bool ODMad::SeisSeqOut::usePar( const IOPar& iop )
{
    return fromPar( iop );
}


bool ODMad::SeisSeqOut::put( const SeisTrc& trc )
{
    if ( !sd_.usable() && !const_cast<SeisSeqOut*>(this)->open() )
	return false;

    errmsg_ = "TODO: Not implemented: write trace";
    return false;
}


bool ODMad::SeisSeqOut::open()
{
    pErrMsg("TODO: open Madagascar output");
    return false;
}


void ODMad::SeisSeqOut::initClass()
{
    Seis::SeqOut::factory().addCreator( create, ODMad::sKeyMadagascar );
}
