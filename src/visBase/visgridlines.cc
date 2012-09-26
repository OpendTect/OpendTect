/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          December 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visgridlines.h"

#include "vispolyline.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "vismaterial.h"
#include "vistransform.h"
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
    , drawstyle_(DrawStyle::create())
    , transformation_(0)
    , gridcs_(false)
    , csinlchanged_(false)
    , cscrlchanged_(false)
    , cszchanged_(false)
{
    inlines_ = crosslines_ = zlines_ = trcnrlines_ = 0;

    addChild( drawstyle_->getInventorNode() );
    setMaterial( 0 );
}


GridLines::~GridLines()
{
    for ( int idx=0; idx<polylineset_.size(); idx++ )
    {
	removeChild( polylineset_[idx]->getInventorNode() );
	polylineset_[idx]->unRef();
	polylineset_.remove(idx--);
    }
}


void GridLines::setLineStyle( const LineStyle& ls )
{
    for ( int idx=0; idx<polylineset_.size(); idx++ )
	polylineset_[idx]->getMaterial()->setColor( ls.color_ );

    drawstyle_->setLineStyle( ls );
}


void GridLines::getLineStyle( LineStyle& ls ) const
{
    if ( polylineset_.size() == 0 ) return;

    ls = drawstyle_->lineStyle();
    ls.color_ = polylineset_[0]->getMaterial()->getColor();
}


void GridLines::setGridCubeSampling( const CubeSampling& cs )
{
    if ( cs==gridcs_ )
	return;

    if ( cs.hrg.inlRange() != gridcs_.hrg.inlRange() || 
	 cs.hrg.step.inl != gridcs_.hrg.step.inl )
	csinlchanged_ = true;
    if ( cs.hrg.crlRange() != gridcs_.hrg.crlRange() ||
	 cs.hrg.step.crl != gridcs_.hrg.step.crl )
	cscrlchanged_ = true;
    if ( cs.zrg != gridcs_.zrg || cs.zrg.step != gridcs_.zrg.step )
	cszchanged_ = true;

    gridcs_ = cs;

    if ( gridcs_.hrg.step.inl == 0 )
	gridcs_.hrg.step.inl = planecs_.hrg.step.inl;
    if ( gridcs_.hrg.step.crl == 0 )
	gridcs_.hrg.step.crl = planecs_.hrg.step.crl;
    if ( mIsZero(gridcs_.zrg.step,mDefEps) )
	gridcs_.zrg.step = planecs_.zrg.step;
}


void GridLines::adjustGridCS()
{
    if ( !planecs_.isDefined() || !gridcs_.isDefined() )
	return;

    const HorSampling& phs = planecs_.hrg;
    HorSampling& ghs = gridcs_.hrg;

    if ( csinlchanged_ )
    {
	while ( phs.start.inl > ghs.start.inl ) 
	    ghs.start.inl += ghs.step.inl;
	
	while ( phs.start.inl < ghs.start.inl - ghs.step.inl )
	    ghs.start.inl -= ghs.step.inl;

	while ( phs.stop.inl > ghs.stop.inl + ghs.step.inl )
	    ghs.stop.inl += ghs.step.inl;

	while ( phs.stop.inl < ghs.stop.inl )
	    ghs.stop.inl -= ghs.step.inl;
	csinlchanged_ = false;
    }

    if ( cscrlchanged_ )
    {
	while ( phs.start.crl > ghs.start.crl )
	    ghs.start.crl += ghs.step.crl;

	while ( phs.start.crl < ghs.start.crl - ghs.step.crl )
	    ghs.start.crl -= ghs.step.crl;

	while ( phs.stop.crl > ghs.stop.crl + ghs.step.crl )
	    ghs.stop.crl += ghs.step.crl;

	while ( phs.stop.crl < ghs.stop.crl )
	    ghs.stop.crl -= ghs.step.crl;
	cscrlchanged_ = false;
    }

    if ( cszchanged_ )
    {
	while ( planecs_.zrg.start > gridcs_.zrg.start )
	    gridcs_.zrg.start += gridcs_.zrg.step;

	while ( planecs_.zrg.start < gridcs_.zrg.start - gridcs_.zrg.step )
	    gridcs_.zrg.start -= gridcs_.zrg.step;
	
	while ( planecs_.zrg.stop > gridcs_.zrg.stop + gridcs_.zrg.step )
	    gridcs_.zrg.stop += gridcs_.zrg.step;

	while ( planecs_.zrg.stop < gridcs_.zrg.stop )
	    gridcs_.zrg.stop -= gridcs_.zrg.step;
	cszchanged_ = false;
    }
}


void GridLines::setPlaneCubeSampling( const CubeSampling& cs )
{
    if ( cs.hrg.inlRange() != planecs_.hrg.inlRange() )
	csinlchanged_ = true;
    if ( cs.hrg.crlRange() != planecs_.hrg.crlRange() )
	cscrlchanged_ = true;
    if ( cs.zrg != planecs_.zrg )
	cszchanged_ = true;

    planecs_ = cs;
    adjustGridCS();
    if ( inlines_ ) drawInlines();
    if ( crosslines_ ) drawCrosslines();
    if ( zlines_ ) drawZlines();
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
    polyline->ref();
    polylineset_ += polyline;
    addChild( polyline->getInventorNode() );
    return polyline;
}


void GridLines::emptyLineSet( IndexedPolyLine* line )
{
    line->removeCoordIndexAfter( -1 );
    line->getCoordinates()->removeAfter( -1 );
}


void GridLines::drawInlines()
{
    if ( !inlines_ ) 
	inlines_ = addLineSet();
    else
	emptyLineSet( inlines_ );

    
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
    if ( !crosslines_ ) 
	crosslines_ = addLineSet();
    else
	emptyLineSet( crosslines_ );
    
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
    if ( !zlines_ ) 
	zlines_ = addLineSet();
    else
	emptyLineSet( zlines_ );
    
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
    transformation_ = tf;
    if ( transformation_ )
	transformation_->ref();
}


void GridLines::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    LineStyle ls;
    getLineStyle( ls );
    BufferString lsstr;
    ls.toString( lsstr );
    par.set( sKeyLineStyle(), lsstr );

    gridcs_.fillPar( par );

    par.setYN( sKeyInlShown(), areInlinesShown() );
    par.setYN( sKeyCrlShown(), areCrosslinesShown() );
    par.setYN( sKeyZShown(), areZlinesShown() );
}


int GridLines::usePar( const IOPar& par )
{
    const int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    gridcs_.usePar( par );

    bool inlshown = false;
    par.getYN( sKeyInlShown(), inlshown );
    showInlines( inlshown );

    bool crlshown = false;
    par.getYN( sKeyCrlShown(), crlshown );
    showCrosslines( crlshown );

    bool zshown = false;
    par.getYN( sKeyZShown(), zshown );
    showZlines( zshown );

    BufferString lsstr;
    if ( par.get(sKeyLineStyle(),lsstr) )
    {
	LineStyle ls;
	ls.fromString( lsstr );
	setLineStyle( ls );
    }

    return 1;
}

} // namespace visBase

