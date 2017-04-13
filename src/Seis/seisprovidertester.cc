/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		April 2017
________________________________________________________________________

-*/

#include "seisprovidertester.h"

#include "seisprovider.h"
#include "seisbuf.h"
#include "seisselectionimpl.h"
#include "od_iostream.h"
#include "seispreload.h"

#define mRetIfNotOK( uirv, printifend ) \
if ( !uirv.isOK() ) \
{ \
   if ( isFinished(uirv) ) \
   { \
       if ( printifend ) \
	   od_cout() << ">At End<" << od_endl; \
   } \
   else \
       od_cout() << uirv << od_endl; \
   \
   return; \
}

#define mResetIfNotCurrentTrc( currenttrc ) \
if ( !currenttrc || trc_.isEmpty() ) \
{ \
    prov_->reset(); \
    mRetIfNotOK( prov_->getNext(trc_), true ); \
}


Seis::ProviderTester::ProviderTester()
     : prov_(0)
{}


uiRetVal Seis::ProviderTester::setInput( const char* dbkystr )
{
    dbky_ = DBKey::getFromString( dbkystr );
    trc_.erase();

    uiRetVal uirv;
    prov_ = Seis::Provider::create( dbky_, &uirv );
    if ( !prov_ )
	od_cout() << uirv << od_endl;

    return uirv;
}


void Seis::ProviderTester::prTrc( const char* start, const uiRetVal& uirv,
				  bool withcomps, bool withoffs )
{
    if ( start )
	od_cout() << start << ": ";

    mRetIfNotOK( uirv, true );

    od_cout() << trc_.info().binID().inl()
	      << '/' << trc_.info().binID().crl()
	      << " #samples=" << trc_.size();
    if ( withcomps )
	od_cout() << " #nrcomps=" << trc_.nrComponents();
    if ( withoffs )
	od_cout() << " O=" << trc_.info().offset_;

    od_cout() << od_endl;
}


void Seis::ProviderTester::prBuf( const char* start, const SeisTrcBuf& tbuf,
				  const uiRetVal& uirv )
{
    if ( start )
	od_cout() << start << ": ";

    mRetIfNotOK( uirv, true );

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


void Seis::ProviderTester::testGetTrc( const TrcKey& tk, const char* start )
{
    if ( !prov_ ) return;

    uiRetVal uirv;
    prov_->get( tk, trc_ );
    prTrc( start, uirv, false, prov_->isPS() );
}


void Seis::ProviderTester::testGetNext()
{
    if ( !prov_ ) return;

    uiRetVal uirv;
    const bool isps = prov_->isPS();
    uirv = prov_->getNext( trc_ );
    prTrc( "First next", uirv, false, isps );
    uirv = prov_->getNext( trc_ );
    prTrc( "Second next", uirv, false, isps );
    uirv = prov_->getNext( trc_ );
    if ( prov_->is2D() )
    {
	const int curlinenr = trc_.info().lineNr();
	while ( trc_.info().lineNr() == curlinenr )
	{
	    uirv = prov_->getNext( trc_ );
	    mRetIfNotOK( uirv, false );
	}
	prTrc( "First on following line", uirv, false, isps );
    }

    od_cout() << od_endl;
}


void Seis::ProviderTester::testSubselection(
		SelData* seldata, const char* txt )
{
    if ( !prov_ || !seldata )
	return;

    od_cout() << txt << od_endl;

    prov_->setSelData( seldata );
    testGetNext();
    prov_->setSelData( 0 );
}


void Seis::ProviderTester::testPreLoadTrc( bool currenttrc )
{
    if ( !prov_ ) return;

    mResetIfNotCurrentTrc( currenttrc );

    TrcKeyZSampling tkzs( true );
    if ( prov_->is2D() )
	tkzs.set2DDef();
    tkzs.hsamp_.start_ = tkzs.hsamp_.stop_ = trc_.info().binID();

    const Pos::GeomID geomid = tkzs.is2D() ? trc_.info().lineNr()
					   : Survey::GM().default3DSurvID();
    Seis::PreLoader pl( dbky_, geomid );
    TextTaskRunner taskrunner( od_cout() );
    pl.setTaskRunner( taskrunner );
    pl.load( tkzs );
    testSubselection( new Seis::RangeSelData(tkzs.hsamp_),
		      "\nSubselection to preloaded data range:" );
    pl.unLoad();
}


void Seis::ProviderTester::testComponentSelection( bool currenttrc )
{
    if ( !prov_ ) return;

    od_cout() << "\nComponent selection:" << od_endl;

    mResetIfNotCurrentTrc( currenttrc );

    uiRetVal uirv;
    BufferStringSet compnms;
    uirv = prov_->getComponentInfo( compnms );
    if ( uirv.isError() )
    {
	od_cout() << uirv << od_endl;
	return;
    }
    const BufferString prstr = compnms.getDispString( 4 );
    od_cout() << prstr << od_endl;

    uirv = prov_->getNext( trc_ );
    prTrc( "\nFirst next", uirv, true );

    prov_->selectComponent( 0 );
    uirv = prov_->get( trc_.info().trckey_, trc_ );
    prTrc( "1st comp selected", uirv, true );

    if ( compnms.size() > 1 )
    {
	prov_->selectComponent( 1 );
	uirv = prov_->get( trc_.info().trckey_, trc_ );
	prTrc( "2nd comp selected", uirv, true );

	int comps[2] = { 0, 1 };
	prov_->selectComponents( TypeSet<int>(comps,2) );
	uirv = prov_->get( trc_.info().trckey_, trc_ );
	prTrc( "Both comps selected", uirv, true );
    }

    prov_->selectComponent( -1 );
    uirv = prov_->get( trc_.info().trckey_, trc_ );
    prTrc( "No comp selected", uirv, true );
    od_cout() << od_endl;
}
