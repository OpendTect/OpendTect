/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2007
________________________________________________________________________

-*/

#include "visscenecoltab.h"

#include "coltabmapper.h"
#include "coltabseqmgr.h"
#include "fontdata.h"
#include "paralleltask.h"
#include "vistext.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osgSim/ColorRange>
#include <osg/Version>

// TODO: Wait for OSG to accept Ranojay's submission containing the new class
// variable osgSim::TextProperties::_font and its handling
//
//#if OSG_MIN_VERSION_REQUIRED(3,?,?)
//# include <osgSim/ScalarBar>
//# define mScalarBarType osgSim::ScalarBar
//#else
# include <osgGeo/ScalarBar>
# define mScalarBarType osgGeo::ScalarBar
//#endif

#define mScalarBar static_cast<mScalarBarType*>(osgcolorbar_)

mCreateFactoryEntry( visBase::SceneColTab );

namespace visBase
{

SceneColTab::SceneColTab()
    : VisualObjectImpl( false )
    , osgcolorbar_( new mScalarBarType )
    , width_( 20 )
    , height_( 250 )
    , horizontal_( false )
    , pos_( Bottom )
    , aspratio_( 1 )
    , winx_( 100 )
    , winy_( 100 )
    , fontsize_( 18 )
    , pixeldensity_( getDefaultPixelDensity() )
    , sequence_( ColTab::SeqMGR().getDefault() )
    , mapper_( new ColTab::Mapper )
{
    addChild( osgcolorbar_ );

    //Set it to something to avoid osg to look for own font
    setAnnotFont( FontData() );

    mScalarBar->setOrientation( mScalarBarType::VERTICAL );
    mScalarBar->setTitle( "" );
    mScalarBar->setNumLabels( 5 );

    setSize( width_, height_ );
    updateDisplay();

    mAttachCB( sequence_->objectChanged(), SceneColTab::coltabChgCB );
    mAttachCB( mapper_->objectChanged(), SceneColTab::coltabChgCB );
}


SceneColTab::~SceneColTab()
{
    removeChild( osgcolorbar_ );
}


void SceneColTab::setLegendColor( const Color& col )
{
#define col2f(rgb) float(col.rgb())/255

    mScalarBarType::TextProperties tp = mScalarBar->getTextProperties();
    tp._color = osg::Vec4( col2f(r), col2f(g), col2f(b), 1.0f );
    mScalarBar->setTextProperties( tp );
}


void SceneColTab::setAnnotFont( const FontData& fd )
{
    mScalarBarType::TextProperties tp = mScalarBar->getTextProperties();
    const float sizefactor = pixeldensity_/getDefaultPixelDensity();
    tp._font = OsgFontCreator::create( fd );
    tp._characterSize = fd.pointSize()*sizefactor;
    mScalarBar->setTextProperties( tp );
    fontsize_ = fd.pointSize();
}


void SceneColTab::setPixelDensity( float dpi )
{
    if ( dpi==pixeldensity_ ) return;

    VisualObjectImpl::setPixelDensity( dpi );

    pixeldensity_ = dpi;

    FontData fd;
    fd.setPointSize( fontsize_ );

    setAnnotFont( fd );
}


void SceneColTab::setColTabSequence( const ColTab::Sequence& newseq )
{
    if ( replaceMonitoredRef(sequence_,newseq,this) )
	updateDisplay();
}


void SceneColTab::setColTabMapper( const ColTab::Mapper& newmpr )
{
    if ( replaceMonitoredRef(mapper_,newmpr,this) )
	updateDisplay();
}


void SceneColTab::setOrientation( bool horizontal )
{
    horizontal_ = horizontal;
    mScalarBar->setOrientation( horizontal_ ? mScalarBarType::HORIZONTAL
					    : mScalarBarType::VERTICAL );
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
    mScalarBar->setAspectRatio( aspratio_ );
    mScalarBar->setWidth(  width_ );
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

    requestSingleRedraw();
}


void SceneColTab::setPos( float x, float y )
{
    mScalarBar->setPosition( osg::Vec3( horizontal_? x : x+height_,y,0.0f) );
}


Geom::Size2D<int> SceneColTab::getSize()
{
    Geom::Size2D<int> sz;
    sz.setWidth( horizontal_ ? width_ : height_ );
    sz.setHeight( horizontal_ ? height_ : width_ );
    return sz;
}


void SceneColTab::updateDisplay()
{
    if ( !isOn() )
	return;

    const int nrcols = 256;
    const ColTab::Table table( *sequence_, nrcols, *mapper_ );
    std::vector<osg::Vec4> colors(nrcols);

    mDefParallelCalc2Pars( ColorUpdator, tr("Color update"),
			   std::vector<osg::Vec4>&, colors,
			   const ColTab::Table&, table )
    mDefParallelCalcBody
    (
	,
	const Color col = table_.color( idx );
	colors_[idx] = osg::Vec4( col2f(r), col2f(g), col2f(b), 1.f-col2f(t) )
	,
    )

    ColorUpdator colorupdator( nrcols, colors, table );
    colorupdator.execute();

    const Interval<float> rg( mapper_->getRange() );
    osgSim::ColorRange* osgcolorrange =
	new osgSim::ColorRange( rg.start,
		mIsZero(rg.width(false),mDefEps) ? 1 : rg.stop, colors );

    mScalarBar->setScalarsToColors( osgcolorrange );
    requestSingleRedraw();
}


bool SceneColTab::turnOn( bool yn )
{
    const bool res = VisualObjectImpl::turnOn( yn );
    updateDisplay();
    return res;
}


} // namespace visBase
