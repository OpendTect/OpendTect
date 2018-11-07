/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "survsubgeom.h"
#include "od_istream.h"
#include "oddirs.h"
#include "survgeom.h"
#define private public
#include "survinfo.h"


#include "testprog.h"


static bool testSubGeom2D()
{
    if ( Survey::GM().nrGeometries() < 1 )
	return true;

    /*
    Survey::SubGeometry2D subgeom( Survey::GM().getGeometryByIdx(0)->id() );
    if ( !subgeom.isValid() )
	return true;

    const auto rg = subgeom.lineTrcNrRange();
    const auto step = rg.step * 5;
    const auto start = rg.start + 3 * step;
    const auto delta = 14;
    const auto stop = rg.start + delta * step;
    subgeom.setRange( start, stop, step );

    mRunStandardTest( subgeom.trcNrStart()==start, "Subgeom2D start" );
    const auto nrtrcs = (stop-start)/step + 1;
    mRunStandardTest( subgeom.nrTrcs()==nrtrcs, "Subgeom2D nr rows" );

    const auto trcnr = start + 5*step;
    const auto arridx = subgeom.idx4TrcNr( trcnr );
    const auto backtransftrcnr( subgeom.trcNr4Idx(arridx) );
    mRunStandardTest( backtransftrcnr==trcnr, "Subgeom2D tranforms" );
    */

    return true;
}

static bool testSubGeom3D()
{
    Survey::SubGeometry3D subgeom;
    const auto inlrg = SI().inlRange(false);
    const auto crlrg = SI().crlRange(false);
    const BinID step( inlrg.step*2, crlrg.step*3 );
    const BinID start( inlrg.start + 2*step.inl(), crlrg.start + 5*step.crl() );
    const BinID delta( 13*step.inl()*inlrg.step, 17*step.crl()*crlrg.step );
    const BinID stop( start+delta );
    subgeom.setRange( start, stop, step );

    mRunStandardTest( subgeom.origin()==start, "Subgeom3D origin" );
    const auto nrrows = (stop.inl()-start.inl())/step.inl() + 1;
    mRunStandardTest( subgeom.nrRows()==nrrows, "Subgeom3D nr rows" );
    const auto nrcols = (stop.crl()-start.crl())/step.crl() + 1;
    mRunStandardTest( subgeom.nrCols()==nrcols, "Subgeom3D nr cols" );

    const BinID bid( inlrg.start + 5*step.inl(), crlrg.start + 7*step.crl() );
    const RowCol arridxs( subgeom.idxs4BinID(bid) );
    const BinID backtransfbid( subgeom.binid4Idxs(arridxs) );
    mRunStandardTest( backtransfbid==bid, "Subgeom3D tranforms" );

    return true;
}


#undef mErrRet
#define mErrRet(s) { od_cout() << "Err: " << s << od_endl; ExitProgram(0); }

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    const BufferString lastsurvfnm = GetLastSurveyFileName();
    od_istream strm( lastsurvfnm );
    if ( !strm.isOK() )
	mErrRet( "No last survey file" );

    BufferString survnm;
    strm.getLine( survnm );
    if ( survnm.isEmpty() )
	mErrRet( "Last survey file is empty" );

    /*const SurveyDiskLocation survloc( survnm );
    SurveyInfo::setSurveyLocation( survloc, false );*/

    if ( !testSubGeom3D() )
	return 1;
    if ( SI().has2D() && !testSubGeom2D() )
	return 1;

    return 0;
}
