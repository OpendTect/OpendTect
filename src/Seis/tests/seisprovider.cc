/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2016
-*/

#include "seisprovider.h"
#include "seisinfo.h"
#include "seisbuf.h"
#include "seisselectionimpl.h"
#include "testprog.h"
#include "moddepmgr.h"
#include "seispreload.h"
#include "executor.h"

#include <iostream>

// Using Penobscot as test survey
static const TrcKey tk_1300_1200( BinID(1300,1200) );


static void prTrc( const char* start, const SeisTrc& trc, const uiRetVal& uirv,
		    bool withoffs=false )
{
    if ( start )
	od_cout() << start << ' ';

    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    od_cout() << ">At End<";
	else
	    od_cout() << uirv << od_endl;
    }
    else
    {
	od_cout() << trc.info().binID().inl()
	    << '/' << trc.info().binID().crl()
	    << " #samples=" << trc.size();
	if ( withoffs )
	    od_cout() << " O=" << trc.info().offset_;
	od_cout() << od_endl;
    }
}

static void prBuf( const char* start, const SeisTrcBuf& tbuf,
		    const uiRetVal& uirv )
{
    if ( start )
	od_cout() << start << ' ';

    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    od_cout() << ">At End<";
	else
	    od_cout() << uirv << od_endl;
    }
    else
    {
	const int sz = tbuf.size();
	if ( sz < 1 )
	    od_cout() << ">Empty buf<" << od_endl;
	else
	{
	    const SeisTrc& trc0 = *tbuf.get( 0 );
	    const SeisTrc& trc1 = *tbuf.get( sz-1 );
	    od_cout() << trc0.info().binID().inl()
	    << '/' << trc0.info().binID().crl();
	    od_cout() << " [" << sz << "] O=" << trc0.info().offset_;
	    od_cout() << "-" << trc1.info().offset_ << od_endl;
	}
    }
}


static bool testVol()
{
    od_cout() << "\n\n---- 3D Volume ----\n" << od_endl;
    const DBKey dbky = DBKey::getFromString( "100010.2" );

    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( dbky, &uirv );
    if ( !prov )
    {
	od_cout() << uirv << od_endl;
	return true; // too bad, but let's not make CDash angry
    }

    od_cout() << "From disk:\n" << od_endl;
    SeisTrcBuf tbuf( false );
    SeisTrc trc;
    uirv = prov->getNext( trc );
    prTrc( "First next", trc, uirv );
    uirv = prov->get( tk_1300_1200, trc );
    prTrc( "tk_1300_1200", trc, uirv );

    TrcKeySampling hs;
    hs.start_.inl() = hs.start_.crl() = 500;
    hs.stop_ = hs.start_;
    Seis::RangeSelData rgsd( hs );
    prov->setSubsel( rgsd );
    uirv = prov->getNext( trc );
    prTrc( "First next after subsel", trc, uirv );
    uirv = prov->getNext( trc );
    prTrc( "Second next after subsel", trc, uirv );

    /*
    Seis::PreLoader pl( dbky );
    TextTaskRunner taskrunner( od_cout() );
    pl.setTaskRunner( taskrunner );
    TrcKeyZSampling cs( true );
    cs.hsamp_.start_.inl() = 450;
    cs.hsamp_.stop_.inl() = 550;
    pl.load( cs );
    rgsd.setIsAll( true );
    prov->setSubsel( rgsd );
    uirv = prov->getNext( trc );
    prTrc( "First next after preload", trc, uirv );
    uirv = prov->get( tk_1300_1200, trc );
    prTrc( "tk_1300_1200", trc, uirv );

    pl.unLoad();
    */
    delete prov;
    return true;
}


static bool testPS3D()
{
    od_cout() << "\n\n---- 3D Pre-Stack ----\n" << od_endl;

    const DBKey dbky = DBKey::getFromString( "100010.5" );

    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( dbky, &uirv );
    if ( !prov )
    {
	od_cout() << uirv << od_endl;
	return true;
    }

    SeisTrc trc;
    SeisTrcBuf tbuf( false );
    uirv = prov->getNext( trc );
    prTrc( "First next", trc, uirv, true );
    uirv = prov->get( tk_1300_1200, trc );
    prTrc( "tk_1300_1200", trc, uirv, true );

    BufferStringSet compnms = prov->getComponentInfo();
    BufferString prstr = compnms.getDispString( 4 );
    od_cout() << prstr << od_endl;
    prov->selectComponent( 1 );
    prov->reset();
    uirv = prov->getNext( trc );
    prTrc( "First next after component sel", trc, uirv, true );

    prov->selectComponent( -1 );
    prov->reset();
    uirv = prov->getNextGather( tbuf );
    prBuf( "First next", tbuf, uirv );
    uirv = prov->getGather( tk_1300_1200, tbuf );
    prBuf( "tk_1300_1200", tbuf, uirv );

    delete prov;
    return true;
}


int testMain( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded("Seis");

    if ( !testVol() )
	ExitProgram( 1 );
    if ( !testPS3D() )
	ExitProgram( 2 );

    return ExitProgram( 0 );
}
