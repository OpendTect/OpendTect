/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/

#include "seis2deventsnapper.h"
#include "seisrangeseldata.h"
#include "emhor2dseisiter.h"
#include "emhorizon2d.h"
#include "seisprovider.h"
#include "seistrc.h"
#include "ioobj.h"
#include "uistrings.h"


Seis2DLineEventSnapper::Seis2DLineEventSnapper( const EM::Horizon2D& orghor,
					EM::Horizon2D& newhor, const Setup& su )
    : SeisEventSnapper(su.gate_)
    , orghor_(orghor)
    , newhor_(newhor)
    , seisprov_(0)
{
    if ( !su.ioobj_ ) return;
    geomid_ = su.geomid_;
    auto* seldata = new Seis::RangeSelData( su.geomid_ );
    uiRetVal uirv;
    seisprov_ = Seis::Provider::create( *su.ioobj_, &uirv );
    if ( seisprov_ )
	seisprov_->setSelData( seldata );
    else
	errmsg_ = uirv;
}


Seis2DLineEventSnapper::~Seis2DLineEventSnapper()
{
    delete seisprov_;
}


uiString Seis2DLineEventSnapper::message() const
{
    return errmsg_.isEmpty()
	? uiStrings::phrHandling(uiStrings::sTrace(mPlural)) : errmsg_;
}


uiString Seis2DLineEventSnapper::nrDoneText() const
{
    return uiStrings::phrHandled(uiStrings::sTrace(mPlural));
}


int Seis2DLineEventSnapper::nextStep()
{
    //TODO: Support multiple sections
    if ( !seisprov_ )
	return ErrorOccurred();

    const uiRetVal uirv = seisprov_->getNext( trc_ );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    return Finished();

	errmsg_ = uirv;
	return ErrorOccurred();
    }

    Coord3 coord = orghor_.getPos( geomid_, trc_.info().trcNr() );
    newhor_.setZPos( geomid_, trc_.info().trcNr(),
		    findNearestEvent(trc_,(float) coord.z_), false );
    nrdone_ ++;

    return MoreToDo();
}


SeisEventSnapper2D::SeisEventSnapper2D( const EM::Horizon2D* hor,
					EM::Horizon2D* newhor,const Setup& su )
    : ExecutorGroup("Seis Lineset Iterator", true)
    , orghor_(hor)
    , newhor_(newhor)
    , type_(su.type_)
    , gate_(su.gate_)
{
    hor2diterator_ = new EM::Hor2DSeisLineIterator( *hor );
    while ( hor2diterator_->next() )
    {
	Pos::GeomID lineid = SurvGeom::getGeomID( hor2diterator_->lineName() );
	if ( !lineid.isValid() )
	    continue;
	Seis2DLineEventSnapper::Setup setup( su.seisioobj_, lineid, gate_ );
	Seis2DLineEventSnapper* snapper =
	    new Seis2DLineEventSnapper( *orghor_, *newhor_, setup );
	snapper->setEvent( VSEvent::Type(type_) );
	add( snapper );
    }
}


SeisEventSnapper2D::~SeisEventSnapper2D()
{
    delete hor2diterator_;
}
