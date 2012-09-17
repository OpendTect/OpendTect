/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: seis2deventsnapper.cc,v 1.5 2010/11/15 09:35:45 cvssatyaki Exp $";

#include "seis2deventsnapper.h"
#include "seisselectionimpl.h"
#include "emhor2dseisiter.h"
#include "emhorizon2d.h"
#include "seis2dline.h"
#include "seistrc.h"
#include "ioobj.h"
#include "ioman.h"


Seis2DEventSnapper::Seis2DEventSnapper( const EM::Horizon2D& orghor,
					EM::Horizon2D& newhor, const Setup& su )
    : SeisEventSnapper(su.gate_)
    , orghor_(orghor)
    , newhor_(newhor)
{
    horgeomid_ = S2DPOS().getGeomID( su.ioobj_->name(), su.lk_.lineName() );
    Seis::RangeSelData* seldata = new Seis::RangeSelData( true );
    seldata->lineKey() = su.lk_;
    seisrdr_ = new SeisTrcReader( su.ioobj_ );
    seisrdr_->setSelData( seldata );
    seisrdr_->prepareWork();
}


Seis2DEventSnapper::~Seis2DEventSnapper()
{
    delete seisrdr_;
}


int Seis2DEventSnapper::nextStep()
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
    Coord3 coord = orghor_.getPos( sid, horgeomid_, trc_.info().nr );
    newhor_.setPos( sid, horgeomid_, trc_.info().nr,
	    	    findNearestEvent(trc_,coord.z), false );
    nrdone_ ++;

    return MoreToDo();
}


Seis2DLineSetEventSnapper::Seis2DLineSetEventSnapper( const EM::Horizon2D* hor,
					EM::Horizon2D* newhor,const Setup& su )
    : ExecutorGroup("Seis Lineset Iterator", true)
    , attribnm_(su.attrnm_)
    , orghor_(hor)
    , newhor_(newhor)
    , type_(su.type_)
    , gate_(su.gate_)
{
    hor2diterator_ = new EM::Hor2DSeisLineIterator( *hor );
    while ( hor2diterator_->next() )
    {
	const int lineid = hor2diterator_->lineSetIndex( attribnm_ );
	const LineKey& lk = hor2diterator_->lineSet()->lineKey( lineid );
	const IOObj* ioobj = IOM().get( hor2diterator_->lineSetKey() );
	Seis2DEventSnapper* snapper =
	    new Seis2DEventSnapper( *orghor_, *newhor_,
		    		    Seis2DEventSnapper::Setup(ioobj,lk,gate_) );
	snapper->setEvent( VSEvent::Type(type_) );
	add( snapper );
    }
}


Seis2DLineSetEventSnapper::~Seis2DLineSetEventSnapper()
{
    delete hor2diterator_;
}
