/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visgridlines.h"

#include "vispolyline.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "vismaterial.h"
#include "vistransform.h"
#include "vispolygonoffset.h"
#include "survinfo.h"
#include "draw.h"
#include "iopar.h"

mCreateFactoryEntry( visBase::GridLines );

namespace visBase
{

const char* GridLines::sKeyLineStyle()  { return "Line style"; }
const char* GridLines::sKeyInlShown()   { return "Inlines shown"; }
const char* GridLines::sKeyCrlShown()   { return "Crosslines shown"; }
const char* GridLines::sKeyZShown()     { return "Zlines shown"; }

GridLines::GridLines()
    : VisualObjectImpl(false)
    , drawstyle_( new DrawStyle )
    , transformation_(0)
    , gridcs_(false)
    , csinlchanged_(false)
    , cscrlchanged_(false)
    , cszchanged_(false)
    , linematerial_( new Material )
{
    inlines_ = crosslines_ = zlines_ = trcnrlines_ = 0;
    drawstyle_->ref();
    setMaterial( linematerial_ );

}


GridLines::~GridLines()
{
    for ( int idx=0; idx<polylineset_.size(); idx++ )
    {
	removeChild( polylineset_[idx]->osgNode() );
	polylineset_[idx]->unRef();
	polylineset_.removeSingle(idx--);
    }

    drawstyle_->unRef();
}


void GridLines::setLineStyle( const OD::LineStyle& ls )
{
    for ( int idx=0; idx<polylineset_.size(); idx++ )
	polylineset_[idx]->getMaterial()->setColor( ls.color_ );

    drawstyle_->setLineStyle( ls );
}


void GridLines::getLineStyle( OD::LineStyle& ls ) const
{
    if ( polylineset_.size() == 0 ) return;

    ls = drawstyle_->lineStyle();
    ls.color_ = polylineset_[0]->getMaterial()->getColor();
}


void GridLines::setGridTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    if ( cs==gridcs_ )
	return;

    if ( cs.hsamp_.inlRange() != gridcs_.hsamp_.inlRange() ||
	 cs.hsamp_.step_.inl() != gridcs_.hsamp_.step_.inl() )
	csinlchanged_ = true;
    if ( cs.hsamp_.crlRange() != gridcs_.hsamp_.crlRange() ||
	 cs.hsamp_.step_.crl() != gridcs_.hsamp_.step_.crl() )
	cscrlchanged_ = true;
    if ( cs.zsamp_ != gridcs_.zsamp_ || cs.zsamp_.step != gridcs_.zsamp_.step )
	cszchanged_ = true;

    gridcs_ = cs;

    if ( gridcs_.hsamp_.step_.inl() == 0 )
	gridcs_.hsamp_.step_.inl() = planecs_.hsamp_.step_.inl();
    if ( gridcs_.hsamp_.step_.crl() == 0 )
	gridcs_.hsamp_.step_.crl() = planecs_.hsamp_.step_.crl();
    if ( mIsZero(gridcs_.zsamp_.step,mDefEps) )
	gridcs_.zsamp_.step = planecs_.zsamp_.step;
}


void GridLines::adjustGridCS()
{
    if ( !planecs_.isDefined() || !gridcs_.isDefined() )
	return;

    const TrcKeySampling& phs = planecs_.hsamp_;
    TrcKeySampling& ghs = gridcs_.hsamp_;

    if ( csinlchanged_ )
    {
	while ( phs.start_.inl() > ghs.start_.inl() )
	    ghs.start_.inl() += ghs.step_.inl();
	
	while ( phs.start_.inl() < ghs.start_.inl() - ghs.step_.inl() )
	    ghs.start_.inl() -= ghs.step_.inl();

	while ( phs.stop_.inl() > ghs.stop_.inl() + ghs.step_.inl() )
	    ghs.stop_.inl() += ghs.step_.inl();

	while ( phs.stop_.inl() < ghs.stop_.inl() )
	    ghs.stop_.inl() -= ghs.step_.inl();
	csinlchanged_ = false;
    }

    if ( cscrlchanged_ )
    {
	while ( phs.start_.crl() > ghs.start_.crl() )
	    ghs.start_.crl() += ghs.step_.crl();

	while ( phs.start_.crl() < ghs.start_.crl() - ghs.step_.crl() )
	    ghs.start_.crl() -= ghs.step_.crl();

	while ( phs.stop_.crl() > ghs.stop_.crl() + ghs.step_.crl() )
	    ghs.stop_.crl() += ghs.step_.crl();

	while ( phs.stop_.crl() < ghs.stop_.crl() )
	    ghs.stop_.crl() -= ghs.step_.crl();
	cscrlchanged_ = false;
    }

    if ( cszchanged_ )
    {
	while ( planecs_.zsamp_.start>gridcs_.zsamp_.start )
	    gridcs_.zsamp_.start += gridcs_.zsamp_.step;

	while ( planecs_.zsamp_.start<gridcs_.zsamp_.start-gridcs_.zsamp_.step )
	    gridcs_.zsamp_.start -= gridcs_.zsamp_.step;
	
	while ( planecs_.zsamp_.stop>gridcs_.zsamp_.stop+gridcs_.zsamp_.step )
	    gridcs_.zsamp_.stop += gridcs_.zsamp_.step;

	while ( planecs_.zsamp_.stop<gridcs_.zsamp_.stop )
	    gridcs_.zsamp_.stop -= gridcs_.zsamp_.step;
	cszchanged_ = false;
    }
}


void GridLines::setPlaneTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    if ( cs.hsamp_.inlRange() != planecs_.hsamp_.inlRange() )
	csinlchanged_ = true;
    if ( cs.hsamp_.crlRange() != planecs_.hsamp_.crlRange() )
	cscrlchanged_ = true;
    if ( cs.zsamp_ != planecs_.zsamp_ )
	cszchanged_ = true;

    planecs_ = cs;
    adjustGridCS();
    if ( inlines_ ) drawInlines();
    if ( crosslines_ ) drawCrosslines();
    if ( zlines_ ) drawZlines();
}


void GridLines::addLine( PolyLine& lines, const Coord3& start,
			 const Coord3& stop )
{
    lines.addPoint( start );
    lines.addPoint( stop );
    const int lastidx = lines.size();
    Geometry::RangePrimitiveSet* ps =
	Geometry::RangePrimitiveSet::create();
    Interval<int> range( lastidx-2, lastidx -1);
    ps->setRange( range );
    ps->ref();
    lines.addPrimitiveSet( ps );
}


PolyLine* GridLines::addLineSet()
{
    PolyLine* polyline = PolyLine::create();
    polyline->setMaterial( linematerial_ );
    polyline->ref();
    polyline->removeAllPrimitiveSets();
    polyline->addNodeState( drawstyle_ );
    polyline->setDisplayTransformation( transformation_ );
    polylineset_ += polyline;
    addChild( polyline->osgNode() );
    return polyline;
}


void GridLines::emptyLineSet( PolyLine* line )
{
    line->removeAllPrimitiveSets();
    line->getCoordinates()->setEmpty();
}


void GridLines::drawInlines()
{
    if ( !inlines_ ) 
	inlines_ = addLineSet();
    else
	emptyLineSet( inlines_ );

    
    const TrcKeySampling& ghs = gridcs_.hsamp_;
    for ( int inl=ghs.start_.inl(); inl<=ghs.stop_.inl(); inl+=ghs.step_.inl() )
    {
	addLine( *inlines_, 
		 Coord3(inl,planecs_.hsamp_.start_.crl(),planecs_.zsamp_.start),
		 Coord3(inl,planecs_.hsamp_.stop_.crl(),planecs_.zsamp_.stop) );
    }
    csinlchanged_ = false;
}


void GridLines::drawCrosslines()
{
    if ( !crosslines_ ) 
	crosslines_ = addLineSet();
    else
	emptyLineSet( crosslines_ );
    
    const TrcKeySampling& ghs = gridcs_.hsamp_;
    for ( int crl=ghs.start_.crl(); crl<=ghs.stop_.crl(); crl+=ghs.step_.crl() )
    {
	addLine( *crosslines_, 
		 Coord3(planecs_.hsamp_.start_.inl(),crl,planecs_.zsamp_.start),
		 Coord3(planecs_.hsamp_.stop_.inl(),crl,planecs_.zsamp_.stop) );
    }
    if ( crosslines_ )
	crosslines_->dirtyCoordinates();

    cscrlchanged_ = false;
}


void GridLines::drawZlines() 
{
    if ( !zlines_ ) 
	zlines_ = addLineSet();
    else
	emptyLineSet( zlines_ );
    
    const TrcKeySampling& phs = planecs_.hsamp_;
    for ( int zidx=0; zidx<gridcs_.zsamp_.nrSteps()+1; zidx++ )
    {
	const float zval = gridcs_.zsamp_.atIndex( zidx );
	addLine( *zlines_, Coord3(phs.start_.inl(),phs.start_.crl(),zval),
			   Coord3(phs.stop_.inl(),phs.stop_.crl(),zval) );
    }
    cszchanged_ = false;
}


void GridLines::showInlines( bool yn )
{
    if ( yn && ( !inlines_ || csinlchanged_ ) )
	drawInlines();
    if ( inlines_ )
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
    if ( crosslines_ )
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
    if ( zlines_ )
	zlines_->turnOn( yn );
}


bool GridLines::areZlinesShown() const
{
    return zlines_ && zlines_->isOn();
}


void GridLines::setDisplayTransformation( const mVisTrans* tf )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_  = tf;

    if ( transformation_ )
	transformation_->ref();

    for ( int idx=0; idx<polylineset_.size(); idx++ )
    {
	if ( polylineset_[idx] )
	    polylineset_[idx]->setDisplayTransformation( transformation_ );
    }
}


void GridLines::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    if ( inlines_ ) inlines_->setPixelDensity( dpi );
    if ( crosslines_ ) crosslines_->setPixelDensity( dpi );
    if ( zlines_ ) zlines_->setPixelDensity( dpi );
    if ( trcnrlines_ ) trcnrlines_->setPixelDensity( dpi );
    
    for ( int idx =0; idx< polylineset_.size(); idx++ )
	polylineset_[idx]->setPixelDensity( dpi );
  
}


} // namespace visBase
