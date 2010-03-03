/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : March 2009
-*/

static const char* rcsID = "$Id: vispointsetdisplay.cc,v 1.11 2010-03-03 10:11:57 cvssatyaki Exp $";

#include "randcolor.h"
#include "selector.h"
#include "viscoord.h"
#include "vispointsetdisplay.h"

#include "datapointset.h"
#include "vispointset.h"
#include "vismaterial.h"


mCreateFactoryEntry( visSurvey::PointSetDisplay );


namespace visSurvey {


PointSetDisplay::PointSetDisplay()
    : VisualObjectImpl( true )
    , data_(0)
{
    setMaterial( 0 );
    colors_ += Color::DgbColor();
    setNrPointSets( 1 );
}


PointSetDisplay::~PointSetDisplay()
{
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
	    pointsets_ += pst;
	}
	else
	    pointsets_.remove( pointsets_.size()-1 );
    }

    while ( nr != pointsets_.size() )
    {
	if ( nr > pointsets_.size() )
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

	Color col = colors_[selgrp];
	std::cout<<selgrp<<"\t"<<(int)col.r()<<"\t"<<(int)col.g()<<"\t"<<(int)col.b()<<std::endl;
	pointsets_[selgrp]->getCoordinates()->addPos(
		Coord3(data_->coord(idx),data_->z(idx)) );
    }
}


void PointSetDisplay::removeSelection( const Selector<Coord3>& selector )
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
    for ( int idx=0; idx<pointsets_.size(); idx++ )
	pointsets_[idx]->setDisplayTransformation( nt );
}


visBase::Transformation* PointSetDisplay::getDisplayTransformation()
{ return pointsets_[0]->getDisplayTransformation(); }

} //namespace visSurvey
