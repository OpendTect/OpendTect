/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : March 2009
-*/

static const char* rcsID = "$Id: vispointsetdisplay.cc,v 1.13 2011-04-28 07:00:12 cvsbert Exp $";

#include "randcolor.h"
#include "selector.h"
#include "viscoord.h"
#include "visevent.h"
#include "vispointsetdisplay.h"

#include "datapointset.h"
#include "vispointset.h"
#include "vismaterial.h"


mCreateFactoryEntry( visSurvey::PointSetDisplay );


namespace visSurvey {


PointSetDisplay::PointSetDisplay()
    : VisualObjectImpl( true )
    , data_(0)
    , transformation_(0)
    , eventcatcher_(0)
    , selpointsetidx_(-1)
{
    setMaterial( 0 );
    colors_ += Color::DgbColor();
    setNrPointSets( 1 );
}


PointSetDisplay::~PointSetDisplay()
{
    setSceneEventCatcher(0);

    for ( int idx=0; idx<pointsets_.size(); idx++ )
	removeChild( pointsets_[idx]->getInventorNode() );
    if ( data_ )
	DPM( DataPackMgr::PointID() ).release( data_->id() );
}


void PointSetDisplay::setNrPointSets( int nr )
{
    while ( nr != pointsets_.size() )
    {
	if ( nr > pointsets_.size() )
	{
	    visBase::PointSet* pst = visBase::PointSet::create();
	    pst->setMaterial( visBase::Material::create() );
	    addChild( pst->getInventorNode() );
	    if ( transformation_ )
		pst->setDisplayTransformation( transformation_ );
	    pointsets_ += pst;
	}
	else
	    pointsets_.remove( pointsets_.size()-1 );
    }

    while ( nr != colors_.size() )
    {
	if ( nr > colors_.size() )
	    colors_ += getRandomColor();
	else
	    colors_.remove( colors_.size()-1 );
    }
}


void PointSetDisplay::setColors( const TypeSet<Color>& cols )
{
    colors_ = cols;
    setNrPointSets( colors_.size() );

    for ( int idx=0; idx<pointsets_.size(); idx++ )
	pointsets_[idx]->getMaterial()->setColor( colors_[idx] );

    update();
}


Color PointSetDisplay::getColor( int idx ) const
{ return colors_[idx]; }


void PointSetDisplay::setPointSize( int sz )
{
    for ( int idx=0; idx<pointsets_.size(); idx++ )
	pointsets_[idx]->setPointSize( sz );
}

int PointSetDisplay::getPointSize() const
{ return pointsets_[0]->getPointSize(); }

bool PointSetDisplay::setDataPack( int dpsid )
{
    DataPack* pack = DPM( DataPackMgr::PointID() ).obtain( dpsid );
    if ( !pack ) return false;

    mDynamicCastGet(DataPointSet*,data,pack)
    data_ = data;
    update();

    return true;
}


void PointSetDisplay::update()
{
    for ( int idx=0; idx<pointsets_.size(); idx++ )
    {
	pointsets_[idx]->getCoordinates()->removeAfter(-1);
	pointsets_[idx]->getMaterial()->setColor( colors_[idx] );
    }

    for ( int idx=0; idx<data_->size(); idx++ )
    {
	if ( !data_->isSelected(idx) )
	    continue;

	int selgrp = data_->selGroup(idx);
	if ( !pointsets_.validIdx(selgrp) )
	    selgrp = 0;

	pointsets_[selgrp]->getCoordinates()->addPos(
		Coord3(data_->coord(idx),data_->z(idx)) );
    }
}


void PointSetDisplay::removeSelection( const Selector<Coord3>& selector,
       					TaskRunner* )
{
    if ( !selector.isOK() )
	return;

    for ( int idx=0; idx<pointsets_.size(); idx++ )
    {
	visBase::PointSet* pointset = pointsets_[idx];
	for ( int idy=0; idy<pointset->getCoordinates()->size(true); idy++ )
	{
	    Coord3 pos = pointset->getCoordinates()->getPos( idy );
	    if ( selector.includes(pos) )
	    {
		DataPointSet::RowID rid = data_->find( DataPointSet::Pos(pos) );
		if ( rid < 0 )
		    continue;
		data_->setSelected( rid, false );
	    }
	}
    }

    update();
}


void PointSetDisplay::setDisplayTransformation( visBase::Transformation* nt )
{
    if ( transformation_ == nt )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();

    for ( int idx=0; idx<pointsets_.size(); idx++ )
	pointsets_[idx]->setDisplayTransformation( transformation_ );
}


visBase::Transformation* PointSetDisplay::getDisplayTransformation()
{ return transformation_; }


void PointSetDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,PointSetDisplay,eventCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = nevc;

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,PointSetDisplay,eventCB));
	eventcatcher_->ref();
    }
}

void PointSetDisplay::eventCB( CallBacker* cb )
{
    if ( !isOn() || isLocked() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    if ( eventinfo.buttonstate_ != OD::RightButton )
	return;

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const DataObject* pickedobj =
	    visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	mDynamicCastGet(const visBase::PointSet*,pointset,pickedobj);
	if ( !pointset ) continue;

	for ( int pidx=0; pidx<pointsets_.size(); pidx++ )
	{
	    if ( pointsets_[pidx] == pointset )
		selpointsetidx_ = pidx;
	}
    }

//    eventcatcher_->setHandled();
}


} //namespace visSurvey
