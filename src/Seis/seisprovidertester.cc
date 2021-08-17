/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		April 2017
________________________________________________________________________

-*/

#include "seisprovidertester.h"

#include "cubesubsel.h"
#include "dbman.h"
#include "filepath.h"
#include "ioobj.h"
#include "linesubsel.h"
#include "od_iostream.h"
#include "posinfo2d.h"
#include "seis2ddata.h"
#include "seisprovider.h"
#include "seisbuf.h"
#include "seisrangeseldata.h"
#include "seispreload.h"
#include "seistrc.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "trckeyzsampling.h"

static bool quiet_ = false;
#define __test_lib_source__ 1
#include "testprog.h"

BinID Seis::ProviderTester::existingbid_( 430, 1006 );
BinID Seis::ProviderTester::nonexistingbid_( 430, 1004 );
Pos::GeomID Seis::ProviderTester::nonexistinggeomid_( 257 );
int Seis::ProviderTester::nonexistingtrcnr_ = 620;



Seis::ProviderTester::ProviderTester( bool bequiet, const char* survnm )
    : trc_(*new SeisTrc)
    , gath_(*new SeisTrcBuf(true))
    , lastgoodtk_(*new TrcKey())
{
    quiet_ = bequiet;
    setSurveyName( survnm && *survnm ? survnm : survName() );
    existinggeomid_ = lineGeomID( 0 );
}


Seis::ProviderTester::ProviderTester( GeomType gt, bool bequiet,
				      const char* survnm )
    : ProviderTester(bequiet,survnm)
{
    if ( Seis::is2D(gt) )
	setInput( gt == Line ? lineDBKey() : linePSDBKey() );
    else
	setInput( gt == Vol ? volDBKey() : volPSDBKey() );
}


Seis::ProviderTester::~ProviderTester()
{
    delete prov_;
    delete &lastgoodtk_;
    delete &trc_;
    delete &gath_;
}


Pos::GeomID Seis::ProviderTester::lineGeomID( int idx )
{
    return SurvGeom::getGeomID( BufferString(lineGeomNameBase(),idx) );
}


int Seis::ProviderTester::existingTrcNr( GeomID gid ) const
{
    if ( !gid.isValid() )
	gid = existinggeomid_;

    PtrMan<IOObj> ioobj = getIOObj( prov_->dbKey() );
    if ( ioobj )
    {
	Seis2DDataSet ds2d( *ioobj );
	PosInfo::Line2DData l2dd;
	auto uirv = ds2d.getGeometry( gid, l2dd );
	if ( uirv.isOK() && !l2dd.isEmpty() )
	    return l2dd.trcNr( l2dd.size() / 2 );
    }

    return 600; // who knows

}


void Seis::ProviderTester::setSurveyName( const char* survname )
{
    File::Path fp( SI().getFullDirPath() );
    fp.setFileName( survname );
    uirv_ = DBM().setDataSource( fp.fullPath() );
}


void Seis::ProviderTester::setInput( const DBKey& dbky )
{
    trc_.erase();
    gath_.setEmpty();

    delete prov_;
    prov_ = Seis::Provider::create( dbky, &uirv_ );
    if ( !prov_ )
	errStream() << uirv_.getText() << od_endl;
}



void Seis::ProviderTester::prTrc( const SeisTrc* trc, const char* start ) const
{
    if ( quiet_ )
	return;
    od_ostream& strm = logStream();

    if ( !trc )
	trc = &trc_;

    strm << '\t';
    if ( start )
	strm << start;

    const auto& ti = trc->info();
    strm << ti.trcKey().usrDispStr();
    if ( prov_->isPS() )
	strm << '`' << ti.offset_;

    strm << " ns=" << trc->size();
    if ( !prov_->isPS() )
	strm << " nrcomps=" << trc->nrComponents();
    if ( trc->isNull() )
	strm << " [null trace]";
    strm << od_endl;
}


void Seis::ProviderTester::prGath( const SeisTrcBuf* tbuf ) const
{
    if ( quiet_ )
	return;
    od_ostream& strm = logStream();
    if ( !tbuf )
	tbuf = &gath_;

    const int sz = tbuf->size();
    strm << "Current gather:" << od_endl;
    if ( sz < 1 )
	strm << "<Empty Gather>" << od_endl;
    else
	strm << "Gather size: " << sz << od_endl;
    for ( auto trc : *tbuf )
	prTrc( trc, "\t" );
}


void Seis::ProviderTester::prResult( const char* tstnm, bool isok,
					bool istrc ) const
{
    BufferString dispnm( tstnm );
    if ( prov_->isPS() )
	dispnm.add( " [" ).add( istrc ? "trace" : "gather" ).add( "]" );
    handleTestResult( isok, dispnm );
}


#define mHandleTstRes( tstnm, res, istrc ) \
    prResult( tstnm, res, istrc ); \
    if ( !(res) ) \
    { \
	if ( !uirv_.isOK() ) \
	     logStream() << uirv_.getText() << od_endl; \
	return false; \
    }

#define mHandleUiRv( tstnm, wantok, istrc ) \
    mHandleTstRes( tstnm, uirv_.isOK() == wantok, istrc )

#define mHandleUiRvFailure( tstnm ) \
    if ( !uirv_.isOK() ) \
    { \
	errStream() << uirv_.getText() << od_endl; \
	return false; \
    }

#define mHandleProvReset() \
    uirv_ = prov_->reset(); \
    mHandleUiRvFailure( "Reset Provider" )

#define mExistStr() exists ? " [Existing]" : " [Non-exisiting]"


bool Seis::ProviderTester::testGetAt( const TrcKey& tk, bool exists )
{
    const BufferString tstnm( "Get At ", tk.usrDispStr(), mExistStr() );
    uirv_ = prov_->getAt( tk, trc_ );
    mHandleUiRv( tstnm, exists, true );
    if ( exists )
	lastgoodtk_ = tk;

    if ( prov_->isPS() )
    {
	uirv_ = prov_->getGatherAt( tk, gath_ );
	mHandleUiRv( tstnm, exists, false );
    }

    return testGetViaSD( tk, exists );
}


bool Seis::ProviderTester::testGetViaSD( const TrcKey& tk, bool exists )
{
    const BufferString tstnm( "Get Using SelData: ", tk.usrDispStr(),
			      mExistStr() );
    mHandleProvReset();

    SelData* prevsd = prov_->selData() ? prov_->selData()->clone() : 0;
    prov_->setSelData( new RangeSelData(tk) );
    prov_->commitSelections();

    if ( exists )
    {
	const auto expectednrtrcs = prov_->totalNr();
	mHandleTstRes( "Expecting one position", expectednrtrcs==1, true )
    }

    uirv_ = prov_->getNext( trc_ );
    mHandleUiRv( tstnm, exists, true );
    if ( exists )
	prTrc();

    if ( prov_->isPS() )
    {
	mHandleProvReset();
	uirv_ = prov_->getNextGather( gath_ );
	mHandleUiRv( tstnm, exists, false );
	if ( exists )
	    prGath();
    }

    prov_->setSelData( prevsd );
    return true;
}


bool Seis::ProviderTester::testGetAll()
{
    static const char* tstnm = "Get All";

    mHandleProvReset();
    od_int64 nrtrcs = 0;
    auto expectednrtrcs = prov_->totalNr();
    while ( true )
    {
	uirv_ = prov_->getNext( trc_ );
	if ( !uirv_.isOK() )
	    break;
	nrtrcs++;
    }

    mHandleTstRes( tstnm, isFinished(uirv_), true )
    uirv_.setOK();
    if ( !prov_->is2D() )
	{ mHandleTstRes( "Expected nr traces", nrtrcs==expectednrtrcs, true ); }

    if ( prov_->isPS() )
    {
	mHandleProvReset();
	while ( uirv_.isOK() )
	    uirv_ = prov_->getNextGather( gath_ );
	mHandleTstRes( tstnm, isFinished(uirv_), false )
	uirv_.setOK();
    }

    return true;
}


Seis::PreLoader* Seis::ProviderTester::preLoad( const CubeSubSel& css ) const
{
    auto* pl = new PreLoader( prov_->dbKey() );

    if ( !prov_->isPS() )
	pl->load( &css );
    else
    {
	const auto linerg = css.inlRange();
	pl->loadPS3D( &linerg );
    }
    return pl;
}


Seis::PreLoader* Seis::ProviderTester::preLoad(
				    const LineSubSelSet& lsss ) const
{
    auto* pl = new PreLoader( prov_->dbKey() );

    const bool isps = prov_->isPS();
    ObjectSet<GeomSubSel> gsss;
    for ( auto lss : lsss )
    {
	if ( isps )
	    pl->loadPS2D( lss->geomID().name() );
	else
	    gsss += lss;
    }

    if ( !isps )
	pl->load( gsss );

    return pl;
}


void Seis::ProviderTester::removePreLoad( PreLoader* pl ) const
{
    if ( pl )
    {
	pl->unLoad();
	delete pl;
    }
}


bool Seis::ProviderTester::testIOParUsage()
{
    IOPar iop;
    prov_->fillPar( iop );

    mHandleProvReset();
    if ( !prov_->goTo(lastgoodtk_) )
	mHandleUiRv( "Provider goTo for usePar", true, true );

    TrcKey provpos;
    prov_->getCurPosition( provpos );
    mHandleTstRes( "Result of goTo", provpos==lastgoodtk_, true )
    uirv_ = prov_->usePar( iop );
    mHandleUiRv( "Provider restore via usePar", true, true );

    prov_->getCurPosition( provpos );
    mHandleTstRes( "Position restoration after usePar",
		    provpos==lastgoodtk_, true )
    return true;
}
