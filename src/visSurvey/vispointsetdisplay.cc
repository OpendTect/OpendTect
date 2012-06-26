/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : March 2009
-*/

static const char* rcsID mUnusedVar = "$Id: vispointsetdisplay.cc,v 1.19 2012-06-26 08:59:34 cvssatyaki Exp $";

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
    , dpsdispprop_(0)
{
    setMaterial( 0 );
    setPointSet();
}


PointSetDisplay::~PointSetDisplay()
{
    setSceneEventCatcher(0);

    removeChild( pointset_->getInventorNode() );
    if ( data_ )
	DPM( DataPackMgr::PointID() ).release( data_->id() );
    delete dpsdispprop_;
}


void PointSetDisplay::setPointSet()
{
    visBase::PointSet* pst = visBase::PointSet::create();
    pst->setMaterial( visBase::Material::create() );
    pst->setMaterialBinding(
	    visBase::Shape::cPerVertexMaterialBinding() );
    addChild( pst->getInventorNode() );
    if ( transformation_ )
	pst->setDisplayTransformation( transformation_ );
    pointset_ = pst;
}


void PointSetDisplay::setDispProp( const DataPointSetDisplayProp* prop )
{
    delete dpsdispprop_;
    dpsdispprop_ = prop->clone();
}


void PointSetDisplay::setPointSize( int sz )
{
    pointset_->setPointSize( sz );
}

int PointSetDisplay::getPointSize() const
{ return pointset_->getPointSize(); }


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
    pointset_->getCoordinates()->removeAfter(-1);

    for ( int idx=0; idx<data_->size(); idx++ )
    {
	if ( dpsdispprop_->showSelected() && !data_->isSelected(idx) )
	    continue;

	Color col;
	if ( dpsdispprop_->showSelected() )
	{
	    int selgrp = data_->selGroup(idx);
	    col = dpsdispprop_->getColor( (float)selgrp );
	}
	else
	{
	    const float val = data_->value( dpsdispprop_->dpsColID(), idx );
	    if ( mIsUdf(val) )
		continue;

	    col = dpsdispprop_->getColor( val );
	}

	const int ptidx = pointset_->getCoordinates()->addPos(
				Coord3(data_->coord(idx),data_->z(idx)) );
	pointset_->getMaterial()->setColor( col, ptidx );
	const float transp = (float)col.t();
	pointset_->getMaterial()->setTransparency( transp/(float)255, ptidx );
    }
}


void PointSetDisplay::removeSelection( const Selector<Coord3>& selector,
       					TaskRunner* )
{
    if ( !selector.isOK() )
	return;

    for ( int idy=0; idy<pointset_->getCoordinates()->size(true); idy++ )
    {
	Coord3 pos = pointset_->getCoordinates()->getPos( idy );
	if ( selector.includes(pos) )
	{
	    DataPointSet::RowID rid = data_->find( DataPointSet::Pos(pos) );
	    if ( rid < 0 )
		continue;
	    if ( dpsdispprop_->showSelected() )
		data_->setSelected( rid, -1 );
	    else
		data_->setValue( dpsdispprop_->dpsColID(), rid, mUdf(float) );
	}
    }

    update();
}


void PointSetDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ == nt )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();

    pointset_->setDisplayTransformation( transformation_ );
}


const mVisTrans* PointSetDisplay::getDisplayTransformation() const
{ return transformation_; }


void PointSetDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher_ ) eventcatcher_->unRef();

    if ( !nevc ) return;

    eventcatcher_ = nevc;
    eventcatcher_->ref();
}

/*void PointSetDisplay::eventCB( CallBacker* cb )
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

	//if ( pointset_ == pointset )
	    selpointsetidx_ = pidx;
    }

}*/


} //namespace visSurvey
