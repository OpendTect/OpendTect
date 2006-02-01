/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          Decemebr 2005
 RCS:           $Id: visgridlines.cc,v 1.1 2006-02-01 14:25:54 cvsnanne Exp $
________________________________________________________________________

-*/

#include "visgridlines.h"

#include "vispolyline.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "vismaterial.h"
#include "vistransform.h"
#include "draw.h"

mCreateFactoryEntry( visBase::GridLines );

namespace visBase
{

GridLines::GridLines()
    : VisualObjectImpl(false)
    , drawstyle_(DrawStyle::create())
    , transformation_(0)
{
    inlines_ = crosslines_ = zlines_ = trcnrlines_ = 0;

    addChild( drawstyle_->getInventorNode() );
}


void GridLines::setLineStyle( const LineStyle& ls )
{
    for ( int idx=0; idx<polylineset_.size(); idx++ )
	polylineset_[idx]->getMaterial()->setColor( ls.color );

    drawstyle_->setLineStyle( ls );
}


void GridLines::getLineStyle( LineStyle& ls ) const
{
    if ( polylineset_.size() == 0 ) return;

    ls = drawstyle_->lineStyle();
    ls.color = polylineset_[0]->getMaterial()->getColor();
}


void GridLines::setCubeSampling( const CubeSampling& cs )
{
    cs_ = cs;
}


void GridLines::addLine( IndexedPolyLine& lines, const Coord3& start,
			 const Coord3& stop )
{
    Coordinates* coords = lines.getCoordinates();
    const int startidx = coords->addPos( start );
    const int stopidx = coords->addPos( stop );
    const int nrcrdidx = lines.nrCoordIndex();
    lines.setCoordIndex( nrcrdidx, startidx );
    lines.setCoordIndex( nrcrdidx+1, stopidx );
    lines.setCoordIndex( nrcrdidx+2, -1 );
}


IndexedPolyLine* GridLines::addLineSet()
{
    IndexedPolyLine* polyline = IndexedPolyLine::create();
    polylineset_ += polyline;
    addChild( polyline->getInventorNode() );
    return polyline;
}


void GridLines::drawCrosslines()
{
    if ( !crosslines_ ) crosslines_ = addLineSet();
    const HorSampling& hs = cs_.hrg;
    for ( int crl=hs.start.crl; crl<=hs.stop.crl; crl+=hs.step.crl )
    {
	addLine( *crosslines_, Coord3(hs.start.inl,crl,cs_.zrg.start),
			       Coord3(hs.stop.inl,crl,cs_.zrg.stop) );
    }
}


void GridLines::drawInlines()
{
    if ( !inlines_ ) inlines_ = addLineSet();
    const HorSampling& hs = cs_.hrg;
    for ( int inl=hs.start.inl; inl<=hs.stop.inl; inl+=hs.step.inl )
    {
	addLine( *inlines_, Coord3(inl,hs.start.crl,cs_.zrg.start),
			    Coord3(inl,hs.stop.crl,cs_.zrg.stop) );
    }
}


void GridLines::drawZlines() 
{
    if ( !zlines_ ) zlines_ = addLineSet();
    const HorSampling& hs = cs_.hrg;
    for ( int zidx=0; zidx<cs_.zrg.nrSteps(); zidx++ )
    {
	const float zval = cs_.zrg.atIndex( zidx );
	addLine( *zlines_, Coord3(hs.start.inl,hs.start.crl,zval),
			   Coord3(hs.stop.inl,hs.stop.crl,zval) );
    }
}


void GridLines::setDisplayTransformation( Transformation* tf )
{
    if ( transformation_ )
	transformation_->unRef();
    transformation_ = tf;
    if ( transformation_ )
	transformation_->ref();
}

} // namespace visBase
