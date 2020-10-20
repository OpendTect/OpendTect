/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2016
-*/

#include "batchprog.h"
#include "testprog.h"

#include "ctxtioobj.h"
#include "cubedata.h"
#include "dbman.h"
#include "moddepmgr.h"
#include "seisbuf.h"
#include "seisprovider.h"
#include "seisprovidertester.h"
#include "seisstorer.h"
#include "seisstorer.h"
#include "seistrc.h"
#include "survgeom2d.h"
#include "survgeommgr.h"
#include "surveydisklocation.h"


/* Program creates Vol, VolPS, Line and LinePS seismic data.

   * Vol contains 5 inlines in the middle of the 'input cube'
   * VolPS contains (fake) PS data on the 1st, 3rd and 5th inline
   * Line contains lines coinciding with the 1st, 3rd and 5th inline
   * LinePS contains PS data for 1st and last line, thus at 1st and 5th inline
*/

typedef int idx_type;
mUseType( Pos, GeomID );
mUseType( Seis, GeomType );
mUseType( Seis, Provider );
mUseType( Seis, Storer );
mUseType( Seis, ProviderTester );
mUseType( Survey, Geometry2D );
mUseType( Geometry2D, trcnr_type );
mUseType( Geometry2D, spnr_type );

static const DBKey inpdatadbky_( "100010.2" );
static SeisTrcBuf traces_( true );
static const char* volnm_ = "IOTest Volume";
static const char* volpsnm_ = "IOTest PS 3D";
static const char* lineattrname_ = "IOTest_Attr";
static const char* linepsnm_ = "IOTest PS 2D";

static const int nrlines_ = 5;
static const int nrtrcsperline_ = 101;
static const trcnr_type starttrcnr_ = 100;
static const float maxoffs_ = 1500.0f;
#define mLineGeomNm(nr) BufferString(ProviderTester::lineGeomNameBase(),nr)


#define mErrRet(s) { tstStream(true) << od_endl << s << od_endl; return false; }
#define mErrRetUiStr(s) mErrRet( toString(s) )
#define mErrRetUirv() mErrRet( toString(uiString(uirv)) )

#define mGetStorer( gt, dbky, nm ) \
    PtrMan<Storer> storerptrman = getStorer( gt, dbky, nm ); \
    if ( !storerptrman ) \
	mErrRet( "Cannot create storer" ) \
    Storer& storer = *storerptrman; \
    uiRetVal uirv

#define mGetTrc(trc) \
    if ( !isPresentTrace(itrc) ) \
	continue; \
    const auto& trc = *traces_.get( itrc )

#define mPutTrc(trc) \
    uirv = storer.put( trc ); \
    if ( !uirv.isOK() ) \
	mErrRetUirv()

#define mWrapUp() \
    uirv = storer.close(); \
    if ( !uirv.isOK() ) \
	mErrRetUirv() \
    tstStream(false) << " [OK]" << od_endl; \
    return true


static bool fillTrcBuf()
{
    uiRetVal uirv;
    PtrMan<Provider> prov = Provider::create( inpdatadbky_, &uirv );
    if ( !uirv.isOK() )
	mErrRetUirv()

    auto& prov3d = *prov->as3D();
    PosInfo::CubeData cd = prov3d.possibleCubeData();
    if ( cd.size() < 5 )
	mErrRet( "Pick a larger input cube" )

    const auto midinlidx = cd.size() / 2;
    const auto inlstepout = nrlines_ / 2;
    for ( auto inloffs=-inlstepout; inloffs<=inlstepout; inloffs++ )
    {
	const auto& ld = *cd.get( midinlidx + inloffs*2 );
	const auto& seg0 = ld.segments_.first();
	const auto midcrl = seg0.center();
	const BinID linestart( ld.linenr_, midcrl-50*seg0.step );
	if ( !prov3d.goTo(linestart) )
	    mErrRet( "Cannot go to line start")

	for ( auto idx=0; idx<nrtrcsperline_; idx++ )
	{
	    auto* trc = new SeisTrc;
	    uirv = idx == 0 ? prov3d.getCurrent( *trc )
			    : prov3d.getNext( *trc );
	    if ( !uirv.isOK() )
		mErrRetUirv()
	    traces_.add( trc );
	}
    }
    return true;
}


static trcnr_type getBufTrcNr( idx_type itrc )
{
    return starttrcnr_ + 2*itrc;
}
static bool isPresentTrace( idx_type itrc )
{
    return itrc % 20;
}


static void fillGeometry( Geometry2D& g2d, idx_type iln )
{
    const auto i0 = iln * 2 * nrtrcsperline_;
    const auto i1 = i0 + nrtrcsperline_;
    for ( auto itrc=i0; itrc<i1; itrc++ )
    {
	const auto& trc = *traces_.get( itrc );
	if ( itrc == i0 )
	    g2d.zRange() = trc.zRange();

	if ( isPresentTrace(itrc) )
	{
	    const auto tnr = getBufTrcNr( itrc );
	    const spnr_type spnr = (spnr_type)(tnr + 0.5);
	    g2d.add( trc.info().coord_, tnr, spnr );
	}
    }
}


static bool createLineGeoms()
{
    uiString errmsg;

    for ( auto igid=0; igid<ProviderTester::nrLineGeometries(); igid++ )
    {
	RefMan<Geometry2D> g2d = new Geometry2D( mLineGeomNm(igid) );
	fillGeometry( *g2d, igid );
	GeomID gid;
	if ( !Survey::GMAdmin().addEntry(g2d,gid,errmsg) )
	    mErrRetUiStr( errmsg )
    }

    return true;
}


static Storer* getStorer( GeomType gt, const DBKey& dbky, const char* objnm )
{
    tstStream(false) << "Creating " << Seis::nameOf(gt) << " ... ";
    tstStream(false).flush();

    PtrMan<IOObjContext> ctxt = Seis::getIOObjContext( gt, false );
    CtxtIOObj ctio( *ctxt );
    ctio.setObj( dbky );
    if ( !ctio.ioobj_ )
	{ ctio.setName( objnm ); ctio.fillObj(); }

    DBM().removeEntry( ctio.ioobj_->key() );
    if ( ctio.ioobj_->key() != dbky )
	DBM().removeEntry( dbky );
    ctio.ioobj_->setKey( dbky );
    ctio.ioobj_->commitChanges();
    ctio.destroyAll();

    auto* ret = new Storer( dbky );
    ret->setCrFrom( GetExecutableName() );
    return ret;
}


static void mkOffsTrc( SeisTrc& trc, float offs )
{
    if ( offs < 0.001f )
	return;
    SeisTrc worktrc( trc );
    const float reloffs = offs / maxoffs_;
    const auto zwidth = trc.zRange().width();
    worktrc.info().sampling_.start = trc.startPos() + zwidth * reloffs * 0.5f;
    worktrc.info().sampling_.step = trc.stepPos() * (1-reloffs);

    const auto sz = trc.size();
    for ( int isamp=0; isamp<sz; isamp++ )
    {
	const auto z = trc.zPos( isamp );
	trc.set( isamp, worktrc.getValue(z,0), 0 );
    }

    trc.info().offset_ = offs;
}


/* Creating variable number of offset traces */
static bool storePSTraces( Storer& storer, SeisTrc& trc, int itrc,
			    uiRetVal& uirv )
{
    mkOffsTrc( trc, 0.f );
    mPutTrc( trc );
    mkOffsTrc( trc, 100.f );
    mPutTrc( trc );
    mkOffsTrc( trc, 150.f );
    mPutTrc( trc );
    if ( itrc > 50 )
    {
	mkOffsTrc( trc, 250.f );
	mPutTrc( trc );
	mkOffsTrc( trc, 300.f );
	mPutTrc( trc );
	if ( itrc > 100 )
	{
	    mkOffsTrc( trc, 450.f );
	    mPutTrc( trc );
	    mkOffsTrc( trc, 550.f );
	    mPutTrc( trc );
	    if ( itrc > 150 )
	    {
		mkOffsTrc( trc, 800.f );
		mPutTrc( trc );
		mkOffsTrc( trc, 1000.f );
		mPutTrc( trc );
	    }
	}
    }
    return true;
}


/* Creating cube with all collected traces (but a gap at each 20th trace) */
static bool createVol()
{
    mGetStorer( Seis::Vol, ProviderTester::volDBKey(), volnm_ );

    for ( auto itrc=0; itrc<traces_.size(); itrc++ )
    {
	mGetTrc( trc );
	mPutTrc( trc );
    }

    mWrapUp();
}


/* Creating data store with same positions as volume but each 2nd inline only */
static bool createVolPS()
{
    mGetStorer( Seis::VolPS, ProviderTester::volPSDBKey(), volpsnm_ );
    for ( auto itrc=0; itrc<traces_.size(); itrc++ )
    {
	const int lineidx = itrc / nrtrcsperline_;
	if ( !(lineidx%2) ) // skip lines 1, 3, ...
	    continue;

	mGetTrc( buftrc );
	SeisTrc trc( buftrc );
	storePSTraces( storer, trc, itrc, uirv );
    }
    mWrapUp();
}


/* Creating data only for line 0, 2 and 4 */
static bool createLines()
{
    mGetStorer( Seis::Line, ProviderTester::lineDBKey(), lineattrname_ );

    const auto gidstart = ProviderTester::lineGeomID( 0 ).getI();
    for ( auto itrc=0; itrc<traces_.size(); itrc++ )
    {
	const int lineidx = itrc / nrtrcsperline_;
	if ( lineidx%2 )
	    continue;
	const int gididx = lineidx/2;

	mGetTrc( buftrc );
	SeisTrc trc( buftrc );
	trc.info().setPos( GeomID(gidstart+gididx), getBufTrcNr(itrc) );
	mPutTrc( trc );
    }

    mWrapUp();
}


/* Creating data only for line 0 and 4 */
static bool createLinePS()
{
    mGetStorer( Seis::LinePS, ProviderTester::linePSDBKey(), linepsnm_ );

    const auto gidstart = ProviderTester::lineGeomID( 0 ).getI();
    for ( auto itrc=0; itrc<traces_.size(); itrc++ )
    {
	const int lineidx = itrc / nrtrcsperline_;
	const int gididx = lineidx/2;
	if ( lineidx%2 || gididx == 1 )
	    continue;

	mGetTrc( buftrc );
	SeisTrc trc( buftrc );
	trc.info().setPos( GeomID(gidstart+gididx), getBufTrcNr(itrc) );
	storePSTraces( storer, trc, itrc, uirv );
    }

    mWrapUp();
}


mLoad1Module("Seis")

bool BatchProgram::doWork( od_ostream& strm )
{
    mInitBatchTestProg();

    if ( !fillTrcBuf() )
	return false;
    else if ( !createVol() )
	return false;
    else if ( !createVolPS() )
	return false;
    else if ( !createLineGeoms() )
	return false;
    else if ( !createLines() )
	return false;
    else if ( !createLinePS() )
	return false;

    return true;
}
