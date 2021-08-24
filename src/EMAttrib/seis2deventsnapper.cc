/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/

#include "seis2deventsnapper.h"
#include "seisselectionimpl.h"
#include "emhor2dseisiter.h"
#include "emhorizon2d.h"
#include "seistrc.h"
#include "ioobj.h"
#include "ioman.h"


Seis2DLineEventSnapper::Seis2DLineEventSnapper( const EM::Horizon2D& orghor,
					EM::Horizon2D& newhor, const Setup& su )
    : SeisEventSnapper(su.gate_)
    , orghor_(orghor)
    , newhor_(newhor)
{
    geomid_ = su.geomid_;
    Seis::RangeSelData* seldata = new Seis::RangeSelData( true );
    seldata->setGeomID( su.geomid_ );
    seisrdr_ = new SeisTrcReader( su.ioobj_ );
    seisrdr_->setSelData( seldata );
    seisrdr_->prepareWork();
}


Seis2DLineEventSnapper::~Seis2DLineEventSnapper()
{
    delete seisrdr_;
}


int Seis2DLineEventSnapper::nextStep()
{
    //TODO: Support multiple sections

    const int res = seisrdr_->get( trc_.info() );
    if ( res == -1 )
	return ErrorOccurred();
    else if ( res == 0 )
	return Finished();

    if ( !seisrdr_->get(trc_) )
	return MoreToDo();

    EM::SectionID sid(0);
    Coord3 coord = orghor_.getPos( sid, geomid_, trc_.info().nr );
    newhor_.setPos( sid, geomid_, trc_.info().nr,
	    	    findNearestEvent(trc_,(float) coord.z), false );
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
	Pos::GeomID lineid = Survey::GM().getGeomID(hor2diterator_->lineName());
	if ( lineid == Survey::GeometryManager::cUndefGeomID() )
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
