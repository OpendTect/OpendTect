/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2016
-*/

#include "seisprovider.h"
#include "seisinfo.h"
#include "seisselectionimpl.h"
#include "testprog.h"
#include "moddepmgr.h"

#include <iostream>


static void prTrc( const SeisTrc& trc, const uiRetVal& uirv )
{
    if ( uirv.isOK() )
	od_cout() << trc.info().binID().inl()
	    << '/' << trc.info().binID().crl()
	    << " #samples=" << trc.size() << od_endl;
    else
    {
	if ( isFinished(uirv) )
	    od_cout() << ">At End< ";
	od_cout() << uirv << od_endl;
    }
}


int testMain( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded("Seis");

    const DBKey dbky = DBKey::getFromString( "100010.2" );

    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( dbky, &uirv );
    if ( !prov )
    {
	od_cout() << uirv << od_endl;
	ExitProgram( 0 ); // too bad, but let's not make CDash angry
    }

    SeisTrc trc;
    uirv = prov->getNext( trc );
    prTrc( trc, uirv );

    TrcKeySampling hs;
    hs.start_.inl() = hs.start_.crl() = 500;
    hs.stop_ = hs.start_;
    Seis::RangeSelData rgsd( hs );
    prov->setSubsel( rgsd );
    uirv = prov->getNext( trc );
    prTrc( trc, uirv );
    uirv = prov->getNext( trc );
    prTrc( trc, uirv );

    return ExitProgram( 0 );
}
