/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          Decemebr 2005
 RCS:           $Id: visgridlines.cc,v 1.2 2006-02-09 07:48:06 cvshelene Exp $
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
    , cs_(false)
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


void GridLines::setCubeSampling( const CubeSampling& cs )
{
    if ( cs==cs_ )
	return;

    if ( cs.hrg.inlRange() != cs_.hrg.inlRange() )
	csinlchanged_ = true;
    if ( cs.hrg.crlRange() != cs_.hrg.crlRange() )
	cscrlchanged_ = true;
    if ( cs.zrg != cs_.zrg )
	cszchanged_ = true;

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


void GridLines::drawInlines()
{
    if ( inlines_ ) delete inlines_;
    inlines_ = addLineSet();
    const HorSampling& hs = cs_.hrg;
    for ( int inl=hs.start.inl; inl<=hs.stop.inl; inl+=hs.step.inl )
    {
	addLine( *inlines_, Coord3(inl,hs.start.crl,cs_.zrg.start),
			    Coord3(inl,hs.stop.crl,cs_.zrg.stop) );
    }
    csinlchanged_ = false;
}


void GridLines::drawCrosslines()
{
    if ( crosslines_ ) delete crosslines_;
	crosslines_ = addLineSet();
    const HorSampling& hs = cs_.hrg;
    for ( int crl=hs.start.crl; crl<=hs.stop.crl; crl+=hs.step.crl )
    {
	addLine( *crosslines_, Coord3(hs.start.inl,crl,cs_.zrg.start),
			       Coord3(hs.stop.inl,crl,cs_.zrg.stop) );
    }
    cscrlchanged_ = false;
}


void GridLines::drawZlines() 
{
    if ( zlines_ ) delete zlines_;
    zlines_ = addLineSet();
    const HorSampling& hs = cs_.hrg;
    for ( int zidx=0; zidx<cs_.zrg.nrSteps(); zidx++ )
    {
	const float zval = cs_.zrg.atIndex( zidx );
	addLine( *zlines_, Coord3(hs.start.inl,hs.start.crl,zval),
			   Coord3(hs.stop.inl,hs.stop.crl,zval) );
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
