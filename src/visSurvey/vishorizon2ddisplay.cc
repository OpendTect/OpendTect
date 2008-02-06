/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: vishorizon2ddisplay.cc,v 1.8 2008-02-06 11:46:30 cvsjaap Exp $
________________________________________________________________________

-*/

#include "vishorizon2ddisplay.h"

#include "emhorizon2d.h"
#include "emmanager.h"
#include "iopar.h"
#include "keystrs.h"
#include "rowcolsurface.h"
#include "visdrawstyle.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vispointset.h"
#include "visseis2ddisplay.h"
#include "viscoord.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::Horizon2DDisplay );


namespace visSurvey
{

Horizon2DDisplay::Horizon2DDisplay()
{
    points_.allowNull(true);
}


Horizon2DDisplay::~Horizon2DDisplay()
{
    for ( int idx=0; idx<sids_.size(); idx++ )
    	removeSectionDisplay( sids_[idx] );

    removeEMStuff();
}


void Horizon2DDisplay::setDisplayTransformation( mVisTrans* nt )
{
    EMObjectDisplay::setDisplayTransformation( nt );

    for ( int idx=0; idx<lines_.size(); idx++ )
	lines_[idx]->setDisplayTransformation(transformation_);

    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	if( points_[idx] )
	    points_[idx]->setDisplayTransformation(transformation_);
    }
}


EM::SectionID Horizon2DDisplay::getSectionID(int visid) const
{
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	if ( lines_[idx]->id()==visid ||
	     (points_[idx] && points_[idx]->id()==visid) )
	    return sids_[idx];
    }

    return -1;
}


bool Horizon2DDisplay::addSection( const EM::SectionID& sid )
{
    visBase::IndexedPolyLine* pl = visBase::IndexedPolyLine::create();
    pl->ref();
    pl->setDisplayTransformation( transformation_ );
    pl->setMaterial( 0 );
    addChild( pl->getInventorNode() );
    lines_ += pl;
    points_ += 0;
    sids_ += sid;

    updateSection( sids_.size()-1 );

    return true;
}


void Horizon2DDisplay::removeSectionDisplay( const EM::SectionID& sid )
{
    const int idx = sids_.indexOf( sid );
    if ( idx<0 ) return;

    removeChild( lines_[idx]->getInventorNode() );
    lines_[idx]->unRef();
    if ( points_[idx] )
    {
	removeChild( points_[idx]->getInventorNode() );
	points_[idx]->unRef();
    }

    points_.remove( idx );
    lines_.remove( idx );
    sids_.remove( idx );
}


void Horizon2DDisplay::updateSection( int idx, const TypeSet<int>* lineidsel )
{
    const EM::SectionID sid = emobject_->sectionID( idx );

    mDynamicCastGet( const Geometry::RowColSurface*, rcs,
	    	     emobject_->sectionGeometry( sid ) );

    visBase::IndexedPolyLine* pl = lines_[idx];
    visBase::PointSet* ps = points_[idx];
    
    const StepInterval<int> rowrg = rcs->rowRange();
    int lcidx = 0;
    int pcidx = 0;
    int ciidx = 0;
    RowCol rc;
    for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
    {
	if ( lineidsel && lineidsel->indexOf(rc.row)<0 )
	    continue;

	const StepInterval<int> colrg = rcs->colRange( rc.row );

	Coord3 prevpos = Coord3::udf();
	int indexinline = 0;
	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	{
	    const Coord3 pos = emobject_->getPos( sid, rc.getSerialized() );

	    // Skip if survey coordinates not available
	    if ( !Coord(pos).isDefined() )
		continue;

	    if ( !pos.isDefined() )
	    {
		if ( indexinline==1 )
		{
		    if ( !ps )
		    {
			ps = visBase::PointSet::create();
			ps->ref();
			points_.replace( idx, ps );
		    }

		    ps->getCoordinates()->setPos( pcidx++, prevpos );
		    indexinline = 0;
		}
		else if ( ciidx && pl->getCoordIndex(ciidx-1)!=-1 )
		{
		    pl->setCoordIndex( ciidx++, -1 );
		    indexinline = 0;
		}
	    }
	    else if ( !indexinline )
	    {
		prevpos = pos;
		indexinline = 1;
	    }
	    else if ( indexinline==1 )
	    {
		pl->getCoordinates()->setPos( lcidx, prevpos );
		pl->setCoordIndex(ciidx++, lcidx++ );

		pl->getCoordinates()->setPos( lcidx, pos );
		pl->setCoordIndex(ciidx++, lcidx++ );
		indexinline++;
	    }
	    else
	    {
		pl->getCoordinates()->setPos( lcidx, pos );
		pl->setCoordIndex(ciidx++, lcidx++ );
		indexinline++;
	    }
	}

	if ( indexinline && pl->getCoordIndex(ciidx-1)!=-1 )
	    pl->setCoordIndex( ciidx++, -1 );
    }

    pl->removeCoordIndexAfter( ciidx-1 );
    pl->getCoordinates()->removeAfter( lcidx-1 );
}


void Horizon2DDisplay::emChangeCB( CallBacker* cb )
{
    EMObjectDisplay::emChangeCB( cb );
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
    {
	if ( !burstalertison_ )
	{
	    for ( int idx=0; idx<sids_.size(); idx++ )
		updateSection( idx );
	}
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::BurstAlert )
    {
	if ( postponedposchanges_ )
	{
	    for ( int idx=0; idx<sids_.size(); idx++ )
		updateSection( idx );
	}
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	getMaterial()->setColor( emobject_->preferredColor() );
    }
}


void Horizon2DDisplay::updateLinesOnSections(
			const ObjectSet<const Seis2DDisplay>& seis2dlist )
{
    mDynamicCastGet( const EM::Horizon2D*, h2d, emobject_ );
    if ( !h2d ) return;

    TypeSet<int> lineidset;
    for ( int idx=0; idx<h2d->geometry().nrLines(); idx++ )
    {
	int lineid = h2d->geometry().lineID( idx );
	const char* linenm = h2d->geometry().lineName(lineid);
	const MultiID& lineset = h2d->geometry().lineSet(lineid);

	for ( int idy=0; idy<seis2dlist.size(); idy++ )
	{
	    if ( !strcmp(seis2dlist[idy]->name(), linenm) &&
		 seis2dlist[idy]->lineSetID()==lineset )
	    {
		lineidset += lineid;
		break;
	    }
	}
    }
    
    TypeSet<int>* lineidsel = displayonlyatsections_ ? &lineidset : 0;
    
    for ( int sidx=0; sidx<sids_.size(); sidx++ )
	updateSection( sids_[sidx], lineidsel );
}


void Horizon2DDisplay::updateSeedsOnSections(
			const ObjectSet<const Seis2DDisplay>& seis2dlist )
{
    for ( int idx=0; idx<posattribmarkers_.size(); idx++ )
    {
	visBase::DataObjectGroup* group = posattribmarkers_[idx];
	for ( int idy=0; idy<group->size(); idy++ )
	{
	    mDynamicCastGet(visBase::Marker*,marker,group->getObject(idy));
	    if ( !marker ) continue;

	    marker->turnOn( !displayonlyatsections_ );
	    Coord3 pos = marker->centerPos(true);
	    for ( int idz=0; idz<seis2dlist.size(); idz++ )
	    {
		const float dist = seis2dlist[idz]->calcDist(pos);
		if ( dist < seis2dlist[idz]->maxDist() )
		{
		    marker->turnOn(true);
		    break;
		}
	    }
	}
    }
}


void Horizon2DDisplay::otherObjectsMoved(
		    const ObjectSet<const SurveyObject>& objs, int movedobj )
{
    bool refresh = movedobj==-1 || movedobj==id();
    ObjectSet<const Seis2DDisplay> seis2dlist;

    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet(const Seis2DDisplay*,seis2d,objs[idx]);
	if ( seis2d )
	{
	    seis2dlist += seis2d;
	    if ( movedobj==seis2d->id() )
		refresh = true;
	}
    }

    if ( !refresh ) return;
    
    updateLinesOnSections( seis2dlist );
    updateSeedsOnSections( seis2dlist );
}


bool Horizon2DDisplay::setEMObject( const EM::ObjectID& newid )
{
    if ( !EMObjectDisplay::setEMObject( newid ) )
	return false;

    getMaterial()->setColor( emobject_->preferredColor() );
    return true;
}


void Horizon2DDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visSurvey::EMObjectDisplay::fillPar( par, saveids );
}


int Horizon2DDisplay::usePar( const IOPar& par )
{
    return visSurvey::EMObjectDisplay::usePar( par );
}


}; // namespace visSurvey
