/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          December 2005
 RCS:           $Id: visgridlines.cc,v 1.3 2006-02-09 13:55:53 cvshelene Exp $
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
    , gridcs_(false)
    , csinlchanged_(false)
    , cscrlchanged_(false)
    , cszchanged_(false)
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


void GridLines::setGridCubeSampling( const CubeSampling& cs )
{
    if ( cs==gridcs_ )
	return;

    if ( cs.hrg.inlRange() != gridcs_.hrg.inlRange() )
	csinlchanged_ = true;
    if ( cs.hrg.crlRange() != gridcs_.hrg.crlRange() )
	cscrlchanged_ = true;
    if ( cs.zrg != gridcs_.zrg )
	cszchanged_ = true;

    gridcs_ = cs;
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
    polyline->setMaterial( Material::create() );
    polylineset_ += polyline;
    addChild( polyline->getInventorNode() );
    return polyline;
}


void GridLines::removeLineSet( IndexedPolyLine* line )
{
    removeChild( line->getInventorNode() );
    polylineset_.remove( polylineset_.indexOf( line ) );
}


void GridLines::drawInlines()
{
    if ( inlines_ ) removeLineSet( inlines_ );
    inlines_ = addLineSet();
    const HorSampling& ghs = gridcs_.hrg;
    for ( int inl=ghs.start.inl; inl<=ghs.stop.inl; inl+=ghs.step.inl )
    {
	addLine( *inlines_, 
		 Coord3(inl,planecs_.hrg.start.crl,planecs_.zrg.start),
		 Coord3(inl,planecs_.hrg.stop.crl,planecs_.zrg.stop) );
    }
    csinlchanged_ = false;
}


void GridLines::drawCrosslines()
{
    if ( crosslines_ ) removeLineSet( crosslines_ );
    crosslines_ = addLineSet();
    const HorSampling& ghs = gridcs_.hrg;
    for ( int crl=ghs.start.crl; crl<=ghs.stop.crl; crl+=ghs.step.crl )
    {
	addLine( *crosslines_, 
		 Coord3(planecs_.hrg.start.inl,crl,planecs_.zrg.start),
		 Coord3(planecs_.hrg.stop.inl,crl,planecs_.zrg.stop) );
    }
    cscrlchanged_ = false;
}


void GridLines::drawZlines() 
{
    if ( zlines_ ) removeLineSet( zlines_ );
    zlines_ = addLineSet();
    const HorSampling& phs = planecs_.hrg;
    for ( int zidx=0; zidx<gridcs_.zrg.nrSteps()+1; zidx++ )
    {
	const float zval = gridcs_.zrg.atIndex( zidx );
	addLine( *zlines_, Coord3(phs.start.inl,phs.start.crl,zval),
			   Coord3(phs.stop.inl,phs.stop.crl,zval) );
    }
    cszchanged_ = false;
}


void GridLines::showInlines( bool yn )
{
    if ( yn && ( !inlines_ || csinlchanged_ ) )
	drawInlines();
    else if ( inlines_ )
	inlines_->turnOn( yn );
}


bool GridLines::areInlinesShown() const
{
    return inlines_ && inlines_->isOn();
}


void GridLines::showCrosslines( bool yn )
{
    if ( yn && ( !crosslines_ || cscrlchanged_ ) )
	drawCrosslines();
    else if ( crosslines_ )
	crosslines_->turnOn( yn );
}


bool GridLines::areCrosslinesShown() const
{
    return crosslines_ && crosslines_->isOn();
}


void GridLines::showZlines( bool yn )
{
    if ( yn && ( !zlines_ || cszchanged_ ) )
	drawZlines();
    else if ( zlines_ )
	zlines_->turnOn( yn );
}


bool GridLines::areZlinesShown() const
{
    return zlines_ && zlines_->isOn();
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
