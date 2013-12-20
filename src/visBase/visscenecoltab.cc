/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visscenecoltab.h"

#include "coltabindex.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "fontdata.h"

#include <osgGeo/ScalarBar>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgSim/ColorRange>

mCreateFactoryEntry( visBase::SceneColTab );

namespace visBase
{

SceneColTab::SceneColTab()
    : VisualObjectImpl(false)
    , osgcolorbar_(new osgGeo::ScalarBar)
    , flipseq_(false)
    , width_(20)
    , height_(200)
    , horizontal_(false)
{
    addChild( osgcolorbar_ );
    osgcolorbar_->setOrientation( osgGeo::ScalarBar::VERTICAL );
    osgcolorbar_->setTitle( "" );
    osgcolorbar_->setNumLabels( 5 );
    
    setSize( width_, height_ );
    setColTabSequence( ColTab::Sequence("") );
}


SceneColTab::~SceneColTab()
{
    removeChild( osgcolorbar_ );
}


void SceneColTab::setLegendColor( const Color& col )
{
#define col2f(rgb) float(col.rgb())/255

    osgGeo::ScalarBar::TextProperties tp = osgcolorbar_->getTextProperties();
    tp._color = osg::Vec4( col2f(r), col2f(g), col2f(b), 1.0f );
    osgcolorbar_->setTextProperties( tp );
}


void SceneColTab::setAnnotFont( const FontData& fd )
{
    osgGeo::ScalarBar::TextProperties tp;
    tp._characterSize = fd.pointSize();
    osgcolorbar_->setTextProperties( tp );
}


void SceneColTab::setColTabSequence( const ColTab::Sequence& ctseq )
{
    if ( sequence_==ctseq )
	return;
    
    sequence_ = ctseq;
    updateSequence();
}


void SceneColTab::setOrientation( bool horizontal )
{
    horizontal_ = horizontal;
    osgcolorbar_->setOrientation( horizontal_ ? osgGeo::ScalarBar::HORIZONTAL
					      : osgGeo::ScalarBar::VERTICAL );
}


void SceneColTab::setWindowSize( int winx, int winy )
{
    winx_ = winx;
    winy_ = winy;
    setPos( getPos() );
}


void SceneColTab::setSize( int w, int h )
{
    width_ = horizontal_ ? w : h;
    height_ = horizontal_ ? h : w;
    aspratio_ = mCast(float,height_) / mCast(float,width_);
    osgcolorbar_->setAspectRatio( aspratio_ );
    osgcolorbar_->setWidth(  width_ );
    setPos( getPos() );
}


void SceneColTab::setPos( Pos pos )
{
    pos_ = pos;
    if ( pos_ == Left )
    {
	setOrientation( false );
	setPos( 10, winy_/2 - width_/2 );
    }
    else if ( pos_ == Right )
    {
	setOrientation( false );
	setPos( winx_-3.5*height_, winy_/2 - width_/2 );
    }
    else if ( pos_ == Top )
    {
	setOrientation( true );
	setPos( winx_/2 - width_/2, winy_ - 3*height_ );
    }
    else if ( pos_ == Bottom )
    {
	setOrientation( true );
	setPos( winx_/2 - width_/2, 10 );
    }
}


void SceneColTab::setPos( float x, float y )
{
    osgcolorbar_->setPosition( osg::Vec3( horizontal_? x : x+height_,y,0.0f) );
}


Geom::Size2D<int> SceneColTab::getSize()
{
    Geom::Size2D<int> sz;
    sz.setWidth( horizontal_ ? width_ : height_ );
    sz.setHeight( horizontal_ ? height_ : width_ );
    return sz;
}


void SceneColTab::updateSequence()
{
    if ( !isOn() )
	return;

    const int nrcols = 256;
    ColTab::IndexedLookUpTable table( sequence_, nrcols );
    std::vector<osg::Vec4> colors(nrcols);

    mDefParallelCalc4Pars( ColorUpdator,
	    		   std::vector<osg::Vec4>&, colors, 
			   const ColTab::IndexedLookUpTable&,table,
			   bool, flipseq,
			   const int&, nrcols )
    mDefParallelCalcBody
    (
    	,
	Color col = table_.colorForIndex( flipseq_ ? nrcols_-idx-1 : idx );
	colors_[idx] = osg::Vec4( col2f(r), col2f(g), col2f(b), col2f(t) )
	,
    )

    ColorUpdator colorupdator( nrcols, colors, table, flipseq_, nrcols );
    colorupdator.execute();

    osgSim::ColorRange* osgcolorrange =
	new osgSim::ColorRange(
	rg_.start, mIsZero(rg_.width(false),mDefEps) ? 1 : rg_.stop, colors );
   
    osgcolorbar_->setScalarsToColors( osgcolorrange );
}


void SceneColTab::setColTabMapperSetup( const ColTab::MapperSetup& ms )
{
    Interval<float> rg = ms.range_;
    if ( rg==rg_ && flipseq_==ms.flipseq_ )
	return;
    
    rg_ = rg;
    flipseq_ = ms.flipseq_;

    updateSequence();
}


bool SceneColTab::turnOn( bool yn )
{
    const bool res = VisualObjectImpl::turnOn( yn );
    updateSequence();
    return res;
}


} // namespace visBase
