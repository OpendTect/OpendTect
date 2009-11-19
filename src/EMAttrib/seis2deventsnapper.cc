/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: seis2deventsnapper.cc,v 1.1 2009-11-19 03:48:34 cvssatyaki Exp $";

#include "seis2deventsnapper.h"
#include "seisselectionimpl.h"
#include "emhor2dseisiter.h"
#include "emhorizon2d.h"
#include "seis2dline.h"
#include "seistrc.h"
#include "ioobj.h"
#include "ioman.h"


Seis2DEventSnapper::Seis2DEventSnapper( EM::Horizon2D& hor, const IOObj* ioobj,
					const LineKey& lk,
					const Interval<float>& gate )
    : SeisEventSnapper(gate)
    , hor_(hor)
{
    horlineidx_ = hor.geometry().lineIndex( lk.lineName() );
    Seis::RangeSelData* seldata = new Seis::RangeSelData( true );
    seldata->lineKey() = lk;
    seisrdr_ = new SeisTrcReader( ioobj );
    seisrdr_->setSelData( seldata );
    seisrdr_->prepareWork();

    posid_.setObjectID( hor_.id() );
    posid_.setSectionID( hor_.sectionID(0) );
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

    BinID bid( horlineidx_, trc_.info().nr );
    posid_.setSubID( bid.getSerialized() );
    Coord3 coord = hor_.getPos( posid_ );
    coord.z = findNearestEvent( trc_, coord.z );
    std::cout<<"Z val :"<<coord.z<<std::endl;
    hor_.setPos( posid_, coord, false );
    nrdone_ ++;

    return MoreToDo();
}


Seis2DLineSetEventSnapper::Seis2DLineSetEventSnapper( EM::Horizon2D* hor,
					const BufferString& attrnm, int type,
       				    	const Interval<float>& gate )
    : ExecutorGroup("Seis Lineset Iterator", true)
    , attribnm_(attrnm)
    , hor_(hor)
    , type_(type)
    , gate_(gate)
{
    hor2diterator_ = new EM::Hor2DSeisLineIterator( *hor );
    while ( hor2diterator_->next() )
    {
	const int lineid = hor2diterator_->lineSetIndex( attribnm_ );
	const LineKey& lk = hor2diterator_->lineSet()->lineKey( lineid );
	const IOObj* ioobj = IOM().get( hor2diterator_->lineSetKey() );
	Seis2DEventSnapper* snapper =
	    new Seis2DEventSnapper( *hor_, ioobj, lk, gate_ );
	snapper->setEvent( VSEvent::Type(type_) );
	add( snapper );
    }
}


Seis2DLineSetEventSnapper::~Seis2DLineSetEventSnapper()
{
    delete hor2diterator_;
}
