/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		April 2017
________________________________________________________________________

-*/

#include "seisprovidertester.h"

#include "dbman.h"
#include "filepath.h"
#include "od_iostream.h"
#include "seisprovider.h"
#include "seisbuf.h"
#include "seisselectionimpl.h"
#include "seispreload.h"
#include "seistype.h"
#include "survinfo.h"

#define mPrintTestResult( testname, withcomps, withoffs ) \
{ \
    prTrc( testname, uirv, withcomps, withoffs, false ); \
    od_cout() << " Failure\n"; \
    return false; \
} \
else \
{ \
    prTrc( testname, uirv, withcomps, withoffs, false ); \
    od_cout() << " Success\n"; \
}

#define mPrintBufTestResult( testname ) \
{ \
    prBuf( testname, tbuf, uirv, false ); \
    od_cout() << " Failure\n"; \
    return false; \
} \
else \
{ \
    prBuf( testname, tbuf, uirv, false ); \
    od_cout() << " Success\n"; \
}

#define mRetIfNotOK( uirv ) \
if ( !uirv.isOK() ) \
{ \
   if ( isFinished(uirv) ) \
       od_cout() << ">At End<"; \
   else \
       od_cout() << uirv; \
   \
   return isFinished( uirv ); \
}

#define mResetIfNotCurrentTrc( currenttrc ) \
if ( !currenttrc || trc_.isEmpty() ) \
{ \
    mRetIfNotOK( prov_->reset() ); \
    mRetIfNotOK( prov_->getNext(trc_) ); \
}


Seis::ProviderTester::ProviderTester()
     : prov_(0)
{}


uiRetVal Seis::ProviderTester::setSurveyName( const char* survname )
{
    File::Path fp( SI().getFullDirPath() );
    fp.setFileName( survname );
    return DBM().setDataSource( fp.fullPath() );
}


uiRetVal Seis::ProviderTester::setInput( const char* dbky )
{
    dbky_ = DBKey::getFromString( dbky );
    trc_.erase();

    uiRetVal uirv;
    prov_ = Seis::Provider::create( dbky_, &uirv );
    if ( !prov_ )
	od_cout() << uirv << od_endl;

    return uirv;
}


bool Seis::ProviderTester::prTrc( const char* start, const uiRetVal& uirv,
				  bool withcomps, bool withoffs,
				  bool addnewline )
{
    if ( start )
	od_cout() << start << ": ";

    mRetIfNotOK( uirv );

    od_cout() << trc_.info().binID().inl()
	      << '/' << trc_.info().binID().crl()
	      << " #samples=" << trc_.size();
    if ( withcomps )
	od_cout() << " #nrcomps=" << trc_.nrComponents();
    if ( withoffs )
	od_cout() << " O=" << trc_.info().offset_;

    if ( addnewline )
	od_cout() << od_endl;

    return true;
}


bool Seis::ProviderTester::prBuf( const char* start, const SeisTrcBuf& tbuf,
				  const uiRetVal& uirv, bool addnewline )
{
    if ( start )
	od_cout() << start << ": ";

    mRetIfNotOK( uirv );

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

    if ( addnewline )
	od_cout() << od_endl;

    return true;
}


bool Seis::ProviderTester::testGet( const TrcKey& tk, const char* start )
{
    if ( !prov_ ) return false;

    uiRetVal uirv;
    uirv = prov_->get( tk, trc_ );
    if ( uirv.isError() )
	mPrintTestResult( start, false, prov_->isPS() );

    if ( prov_->isPS() )
    {
	SeisTrcBuf tbuf( false );
	uirv = prov_->getGather( tk, tbuf );
	if ( uirv.isError() )
	    mPrintBufTestResult( start );
    }

    return true;
}


bool Seis::ProviderTester::testGetNext()
{
    if ( !prov_ ) return false;

    uiRetVal uirv;
    while ( uirv.isOK() )
	uirv = prov_->getNext( trc_ );

    const bool isps = prov_->isPS();
    if ( !isFinished(uirv) )
	mPrintTestResult( "Iteration by calling getNext", false, isps );

    if ( isps )
    {
	mRetIfNotOK( prov_->reset() );

	SeisTrcBuf tbuf( false );
	while ( uirv.isOK() )
	    uirv = prov_->getNextGather( tbuf );

	if ( !isFinished(uirv) )
	    mPrintBufTestResult( "Iteration by calling getNextGather" );
    }

    od_cout() << od_endl;
    return true;
}


bool Seis::ProviderTester::testSubselection(
		SelData* seldata, const char* txt, bool outsidedatarg )
{
    if ( !prov_ || !seldata )
	return false;

    prov_->setSelData( seldata );

    uiRetVal uirv; int nrtrcs = 0;
    const int expectednrtrcs = seldata->expectedNrTraces();
    while ( true )
    {
	uirv = prov_->getNext( trc_ );
	if ( !uirv.isOK() || ++nrtrcs>expectednrtrcs )
	    break;
    }

    if ( (!isFinished(uirv) || nrtrcs!=expectednrtrcs) && !outsidedatarg )
	mPrintTestResult( txt, false, false );

    prov_->setSelData( 0 );

    od_cout() << od_endl;
    return true;
}


bool Seis::ProviderTester::testPreLoadTrc( bool currenttrc )
{
    if ( !prov_ ) return false;

    mResetIfNotCurrentTrc( currenttrc );

    TrcKeyZSampling tkzs( true );
    if ( prov_->is2D() )
	tkzs.set2DDef();
    tkzs.hsamp_.start_ = tkzs.hsamp_.stop_ = trc_.info().binID();

    return testPreLoad( tkzs );
}


bool Seis::ProviderTester::testPreLoad( const TrcKeyZSampling& tkzs )
{
    if ( !prov_ ) return false;

    const Pos::GeomID geomid = tkzs.is2D() ? tkzs.hsamp_.start_.inl()
					   : Survey::GM().default3DSurvID();
    Seis::PreLoader pl( dbky_, geomid );
    LoggedTaskRunner taskrunner( od_cout() );
    pl.setTaskRunner( taskrunner );
    if ( !prov_->isPS() )
	pl.load( tkzs );
    else
    {
	if ( prov_->is2D() )
	    pl.loadPS2D( Survey::GM().getName(geomid) );
	else
	{
	    const StepInterval<int> linerg = tkzs.hsamp_.lineRange();
	    pl.loadPS3D( &linerg );
	}
    }

    const bool res = testSubselection( new Seis::RangeSelData(tkzs.hsamp_),
				       "Subselection to preloaded data range" );
    pl.unLoad();
    return res;
}


bool Seis::ProviderTester::testComponentSelection( bool currenttrc )
{
    if ( !prov_ ) return false;

    od_cout() << "Component selection:" << od_endl;

    mResetIfNotCurrentTrc( currenttrc );

    uiRetVal uirv;
    BufferStringSet compnms;
    uirv = prov_->getComponentInfo( compnms );
    mRetIfNotOK( uirv );

    const BufferString prstr = compnms.getDispString( 4 );
    od_cout() << prstr << "\n\n";

    const int nrcomps = compnms.size();
    uirv = prov_->getNext( trc_ );
    if ( trc_.nrComponents() != nrcomps )
	mPrintTestResult( "No comp selected", true, false );

    prov_->selectComponent( 0 );
    uirv = prov_->get( trc_.info().trckey_, trc_ );
    if ( trc_.nrComponents() != 1 )
	mPrintTestResult( "1st comp selected", true, false );

    if ( nrcomps > 1 )
    {
	prov_->selectComponent( 1 );
	uirv = prov_->get( trc_.info().trckey_, trc_ );
	if ( trc_.nrComponents() != 1 )
	    mPrintTestResult( "2nd comp selected", true, false );

	int comps[2] = { 0, 1 };
	prov_->selectComponents( TypeSet<int>(comps,2) );
	uirv = prov_->get( trc_.info().trckey_, trc_ );
	if ( trc_.nrComponents() != 2 )
	    mPrintTestResult( "Both comps selected", true, false );
    }

    prov_->selectComponent( -1 );
    uirv = prov_->get( trc_.info().trckey_, trc_ );
    if ( trc_.nrComponents() != nrcomps )
	mPrintTestResult( "After removing comp selections", true, false );

    od_cout() << od_endl;
    return true;
}


bool Seis::ProviderTester::testIOParUsage( bool currenttrc )
{
    if ( !prov_ ) return false;

    od_cout() << "IOPar usage:" << od_endl;

    mResetIfNotCurrentTrc( currenttrc );

    uiRetVal uirv; IOPar iop;
    uirv = prov_->fillPar( iop );
    mRetIfNotOK( uirv );

    const TrcKey prevposition = prov_->curPosition();
    uirv = prov_->usePar( iop );
    mRetIfNotOK( uirv );

    if ( prov_->curPosition() != prevposition )
	mPrintTestResult( "Position restoration after usePar", false,false);

    od_cout() << od_endl;
    return true;
}
