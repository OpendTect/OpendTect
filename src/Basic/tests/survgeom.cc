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
#include "cubedata.h"
#include "survgeom2d.h"
#include "survgeommgr.h"
#include "survinfo.h"


#include "testprog.h"


static bool testFillers()
{
    mUseType( PosInfo, LineData );
    mUseType( PosInfo, CubeData );
    mUseType( LineData, Segment );
    LineData ld_ref( 0 );
    LineData ld_fill( 0 );
    PosInfo::LineDataFiller ldfiller( ld_fill );
    mRunStandardTest( ld_fill==ld_ref, "LineDataFiller [0]" );
    ldfiller.add( 23 ).add( 26 ).add( 30 ).add( 31 ).add( 32 ).add( 40 );
    ld_ref.segments_.add( Segment(23,26,3) );
    ld_ref.segments_.add( Segment(30,32,1) );
    ld_ref.segments_.add( Segment(40,40,1) );
    ldfiller.finish();
    mRunStandardTest( ld_fill==ld_ref, "LineDataFiller [1]" );

    ldfiller.reset();
    ldfiller.add( 40 ).add( 40 ).add( 38 );
    ldfiller.finish();
    ld_ref.segments_.setEmpty();
    ld_ref.segments_.add( Segment(40,38,-2) );
    mRunStandardTest( ld_fill==ld_ref, "LineDataFiller [2]" );

    CubeData lcd_ref, lcd_fill;
    PosInfo::LineCollDataFiller lcdfiller( lcd_fill );
    mRunStandardTest( lcd_ref==lcd_fill, "LineCollDataFiller [0]" );
    lcdfiller.add( BinID(1,40) ).add( BinID(1,40) ).add( BinID(1,38) );
    lcdfiller.add( BinID(3,38) ).add( BinID(3,40) ).add( BinID(3,42) )
	     .add( BinID(3,45) );
    lcdfiller.add( BinID(5,35) );
    lcdfiller.finish();
    lcd_ref.add( new LineData(1) ).add( new LineData(3) ).add( new LineData(5));
    lcd_ref.get(0)->segments_.add( Segment(40,38,-2) );
    lcd_ref.get(1)->segments_.add( Segment(38,42,2) ).add( Segment(45,45,2) );
    lcd_ref.get(2)->segments_.add( Segment(35,35,mUdf(int)) );
    mRunStandardTest( lcd_ref==lcd_fill, "LineCollDataFiller [1]" );
    lcdfiller.reset();
    lcd_ref.setEmpty();
    lcdfiller.add( BinID(0,0) );
    lcdfiller.finish();
    lcd_ref.add( new LineData(0) );
    lcd_ref.get(0)->segments_.add( Segment(0,0,mUdf(int)) );
    mRunStandardTest( lcd_ref==lcd_fill, "LineCollDataFiller [2]" );

    return true;
}


static bool testSubSel()
{
    mUseType( Pos, IdxSubSelData );
    mUseType( IdxSubSelData, pos_steprg_type );

    const IdxSubSelData fullss( pos_steprg_type(14,77,7) );
    const IdxSubSelData smallss( pos_steprg_type(21,49,14) );

    IdxSubSelData work( fullss );
    work.limitTo( smallss );
    mRunStandardTest( work.outputPosRange()==smallss.outputPosRange(),
			"SubSel limitTo" );

    work.widenTo( fullss );
    mRunStandardTest( work.outputPosRange()==pos_steprg_type(14,70,14),
			"SubSel widenTo" );

    return true;
}


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
#define mErrRet(s) { od_cout() << "Err: " << s << od_endl; return 0; }

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( SI().name().isEmpty() )
	return 0;

    CubeSampling initcs, initworkcs;
    SI().getCubeSampling( initcs, OD::FullSurvey );
    SI().getCubeSampling( initworkcs, OD::UsrWork );

    CubeSampling survcs( false );
    survcs.hsamp_.set( StepInterval<int>(100,750,1),
		       StepInterval<int>(300,1250,1) );
    survcs.zsamp_.set( 0.f, 1.848f, 0.004f );

    SurveyInfo& si = const_cast<SurveyInfo&>( SI() );
    si.setRanges( survcs );
    si.setWorkRanges( survcs ); //For the sanity of SI().

    if ( !testSubSel() )
	return 1;
    if ( !testSubGeom3D() )
	return 1;
    if ( !testSubGeom2D() )
	return 1;
    if ( !testZSubSel() )
	return 1;
    if ( !testFillers() )
	return 1;

    si.setRanges( initcs );
    si.setWorkRanges( initworkcs ); //For the sanity of SI().

    return 0;
}
