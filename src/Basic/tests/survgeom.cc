/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "linesubsel.h"
#include "cubesubsel.h"
#include "od_istream.h"
#include "oddirs.h"
#include "survgeom2d.h"
#include "survgeommgr.h"
#include "survinfo.h"


#include "testprog.h"


static bool testSubGeom2D()
{
    if ( Survey::GM().nr2DGeometries() < 1 )
	return true;

    LineSubSel lss( Survey::GM().get2DGeometryByIdx(0)->geomID() );

    const auto rg = lss.trcNrRange();
    const auto step = rg.step * 5;
    const auto start = rg.start + 3 * step;
    const auto delta = 14;
    const auto stop = rg.start + delta * step;
    lss.setTrcNrRange( LineSubSel::pos_steprg_type(start,stop,step) );

    mRunStandardTest( lss.trcNrRange().start==start, "LineSubSel start" );
    const auto nrtrcs = (stop-start)/step + 1;
    mRunStandardTest( lss.nrTrcs()==nrtrcs, "LineSubSel nr rows" );

    const auto trcnr = start + 5*step;
    const auto arridx = lss.idx4TrcNr( trcnr );
    const auto backtransftrcnr( lss.trcNr4Idx(arridx) );
    mRunStandardTest( backtransftrcnr==trcnr, "LineSubSel tranforms" );

    return true;
}

static bool testSubGeom3D()
{
    CubeSubSel css;
    const auto inlrg = css.inlRange();
    const auto crlrg = css.crlRange();
    const BinID step( inlrg.step*2, crlrg.step*3 );
    const BinID start( inlrg.start + 2*step.inl(), crlrg.start + 5*step.crl() );
    const BinID delta( 13*step.inl()*inlrg.step, 17*step.crl()*crlrg.step );
    const BinID stop( start+delta );
    css.setRange( start, stop, step );

    mRunStandardTest( css.origin()==start, "CubeSubSel origin" );
    const RowCol rc = css.horSizes();
    const auto nrrows = (stop.inl()-start.inl())/step.inl() + 1;
    mRunStandardTest( rc.row()==nrrows, "CubeSubSel nr rows" );
    const auto nrcols = (stop.crl()-start.crl())/step.crl() + 1;
    mRunStandardTest( rc.col()==nrcols, "CubeSubSel nr cols" );

    const BinID bid( inlrg.start + 5*step.inl(), crlrg.start + 7*step.crl() );
    const RowCol arridxs( css.rowCol4BinID(bid) );
    const BinID backtransfbid( css.binID4RowCol(arridxs) );
    mRunStandardTest( backtransfbid==bid, "CubeSubSel tranforms" );

    return true;
}


static bool testZSubSel()
{
    mUseType( Pos, ZSubSelData );
    mUseType( ZSubSelData, z_steprg_type );

    ZSubSelData zssd( z_steprg_type(0.5f,1.5f,0.1f) );
    zssd.setOutputZRange( 0.7f, 1.1f, 0.2f );
    mRunStandardTest( zssd.size()==3, "Z Subsel easy" );
    zssd.setOutputZRange( 0.68f, 1.77f, 0.32f );
    mRunStandardTest( zssd.size()==3, "Z Subsel hard" );
    zssd.setOutputZRange( -2000.f, 2000.f, 0.21f );
    mRunStandardTest( zssd.size()==6, "Z Subsel out-of-bounds" );

    return true;
}


#undef mErrRet
#define mErrRet(s) { od_cout() << "Err: " << s << od_endl; ExitProgram(0); }

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( SI().name().isEmpty() )
	return 0;

    if ( !testSubGeom3D() )
	return 1;
    if ( !testSubGeom2D() )
	return 1;
    if ( !testZSubSel() )
	return 1;

    return 0;
}
