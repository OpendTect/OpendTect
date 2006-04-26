/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: vishorizon2ddisplay.cc,v 1.1 2006-04-26 21:23:32 cvskris Exp $
________________________________________________________________________

-*/

#include "vishorizon2ddisplay.h"

#include "emhorizon2d.h"
#include "emmanager.h"
#include "iopar.h"
#include "keystrs.h"
#include "rowcolsurface.h"
#include "visdrawstyle.h"
#include "vispolyline.h"
#include "vispointset.h"
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
{}


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


void Horizon2DDisplay::updateSection( int idx )
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
	const StepInterval<int> colrg = rcs->colRange( rc.row );

	Coord3 prevpos = Coord3::udf();
	int indexinline = 0;
	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	{
	    const Coord3 pos = emobject_->getPos( sid, rc.getSerialized() );
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
		else if ( ciidx && pl->getTextureCoordIndex(ciidx-1)!=-1 )
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

	if ( indexinline && pl->getTextureCoordIndex(ciidx-1)!=-1 )
	    pl->setCoordIndex( ciidx++, -1 );
    }

    pl->removeCoordIndexAfter( ciidx-1 );
    pl->getCoordinates()->removeAfter( lcidx-1 );
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
