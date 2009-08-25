/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2006
-*/

static const char* rcsID = "$Id: posimpexppars.cc,v 1.1 2009-08-25 10:09:30 cvsbert Exp $";

#include "posimpexppars.h"
#include "survinfo.h"
#include "keystrs.h"
#include "ioman.h"
#include "iopar.h"


const char* PosImpExpPars::sKeyOffset()	{ return sKey::Offset; }
const char* PosImpExpPars::sKeyScale()		{ return sKey::Scale; }
const char* PosImpExpPars::sKeyTrcNr()		{ return sKey::TraceNr; }


const PosImpExpPars& PosImpExpPars::SVY()
{
    static PosImpExpPars* thinst = 0;
    if ( !thinst )
    {
	thinst = new PosImpExpPars;
	thinst->getFromSI();
	IOM().afterSurveyChange.notify( mCB(thinst,PosImpExpPars,survChg) );
    }
    return *thinst;
}

PosImpExpPars& PosImpExpPars::getSVY()
{
    return const_cast<PosImpExpPars&>( SVY() );
}


void PosImpExpPars::getFromSI()
{
    clear();
    usePar( SI().pars() );
}


const char* PosImpExpPars::fullKey( const char* attr, bool scl )
{
    static BufferString ret;
    ret = IOPar::compKey( sKeyBase(), attr );
    ret = IOPar::compKey( ret.buf(), scl ? sKeyScale() : sKeyOffset() );
    return ret.buf();
}


void PosImpExpPars::usePar( const IOPar& iop )
{
    iop.get( fullKey(sKeyBinID(),true), binidscale_ );
    iop.get( fullKey(sKeyBinID(),false), binidoffs_ );
    iop.get( fullKey(sKeyTrcNr(),true), trcnrscale_ );
    iop.get( fullKey(sKeyTrcNr(),false), trcnroffs_ );
    iop.get( fullKey(sKeyCoord(),true), coordscale_ );
    iop.get( fullKey(sKeyCoord(),false), coordoffs_ );
    iop.get( fullKey(sKeyZ(),true), zscale_ );
    iop.get( fullKey(sKeyZ(),false), zoffs_ );
    iop.get( fullKey(sKeyOffset(),true), offsscale_ );
    iop.get( fullKey(sKeyOffset(),false), offsoffs_ );
}


void PosImpExpPars::adjustBinID( BinID& bid ) const
{
    bid.inl *= binidscale_; bid.crl *= binidscale_;
    bid += binidoffs_;
}


void PosImpExpPars::adjustTrcNr( int& tnr ) const
{
    tnr *= trcnrscale_; tnr += trcnroffs_;
}


void PosImpExpPars::adjustCoord( Coord& crd ) const
{
    crd.x *= coordscale_; crd.y *= coordscale_;
    crd += coordoffs_;
}


void PosImpExpPars::adjustZ( float& z ) const
{
    z *= zscale_; z += zoffs_;
}


void PosImpExpPars::adjustOffset( float& offs ) const
{
    offs *= offsscale_; offs += offsoffs_;
}
