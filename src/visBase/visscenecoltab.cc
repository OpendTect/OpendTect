/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visscenecoltab.h"

#include "coltabindex.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "fontdata.h"
#include "vistext.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Version>
#include <osgGeo/ScalarBar>
#include <osgSim/ColorRange>

mCreateFactoryEntry( visBase::SceneColTab );

constexpr int cMinLabels = 5;

namespace visBase
{

SceneColTab::SceneColTab()
    : VisualObjectImpl(false)
    , osgcolorbar_(new osgGeo::ScalarBar)
    , pixeldensity_(getDefaultPixelDensity())
{
    ref();
    addChild( osgcolorbar_ );

    //Set it to something to avoid osg to look for own font
    setAnnotFont( FontData() );

    osgcolorbar_->setOrientation( osgGeo::ScalarBar::VERTICAL );
    osgcolorbar_->setTitle( "" );
    setColTabSequence( ColTab::Sequence("") );

    setSize( width_, height_ );

    unRefNoDelete();
}


SceneColTab::~SceneColTab()
{
    removeChild( osgcolorbar_ );
}


void SceneColTab::setLegendColor( const OD::Color& col )
{
#define col2f(rgb) float(col.rgb())/255

    osgGeo::ScalarBar::TextProperties tp = osgcolorbar_->getTextProperties();
    tp._color = osg::Vec4( col2f(r), col2f(g), col2f(b), 1.0f );
    osgcolorbar_->setTextProperties( tp );
}


void SceneColTab::setAnnotFont( const FontData& fd )
{
    osgGeo::ScalarBar::TextProperties tp = osgcolorbar_->getTextProperties();
    const float sizefactor = pixeldensity_/getDefaultPixelDensity();
    tp._font = OsgFontCreator::create( fd );
    tp._characterSize = fd.pointSize()*sizefactor;
    osgcolorbar_->setTextProperties( tp );
    fontsize_ = fd.pointSize();
}


void SceneColTab::setPixelDensity( float dpi )
{
    if ( dpi==pixeldensity_ )
	return;

    VisualObjectImpl::setPixelDensity( dpi );

    pixeldensity_ = dpi;

    FontData fd;
    fd.setPointSize( fontsize_ );

    setAnnotFont( fd );
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


void SceneColTab::setSize( int width, int height )
{
    width_ = horizontal_ ? std::clamp( width, ColorBarBounds::minHorWidth(),
					      ColorBarBounds::maxHorWidth() )
			 : std::clamp( height, ColorBarBounds::minVertHeight(),
					      ColorBarBounds::maxVertHeight() );
    height_ = horizontal_ ? std::clamp( height, ColorBarBounds::minHorHeight(),
						ColorBarBounds::maxHorHeight() )
			  : std::clamp( width, ColorBarBounds::minVertWidth(),
					       ColorBarBounds::maxVertWidth() );

    aspratio_ = mCast(float,height_) / mCast(float,width_);
    osgcolorbar_->setAspectRatio( aspratio_ );
    osgcolorbar_->setWidth( width_ );
    setPos( getPos() );

    setNumLabels( sequence_.nrSegments()+1 );
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
	setPos( winx_-(height_*1.5+getMinLabelWidth()), winy_/2 - width_/2 );
    }
    else if ( pos_ == Top )
    {
	setOrientation( true );
	setPos( winx_/2 - width_/2, winy_ - (height_+35) );
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
    osgcolorbar_->setPosition( osg::Vec3( horizontal_? x : x+height_,y,0.0f) );
}


void SceneColTab::setNumLabels( int num )
{
    if ( horizontal_ )
    {
	if ( getMinLabelWidth() <= width_ )
	{
	    const int minlabels = sequence_.hasEqualSegments() ?
				  sequence_.nrSegments()+1 : cMinLabels;
	    osgcolorbar_->setNumLabels( std::max(num,minlabels) );
	}
	else
	    osgcolorbar_->setNumLabels( cMinLabels );
    }
    else
    {
	if ( getMinLabelHeight() <= width_ )
	{
	    const int minlabels = sequence_.hasEqualSegments() ?
				  sequence_.nrSegments()+1 : cMinLabels;
	    osgcolorbar_->setNumLabels( std::max(num,minlabels) );
	}
	else
	    osgcolorbar_->setNumLabels( cMinLabels );
    }
}


Geom::Size2D<int> SceneColTab::getSize()
{
    Geom::Size2D<int> sz;
    sz.setWidth( horizontal_ ? width_ : height_ );
    sz.setHeight( horizontal_ ? height_ : width_ );
    return sz;
}


int SceneColTab::getLabelCharSize()
{
    return osgcolorbar_->getTextProperties()._characterSize;
}


int SceneColTab::getNumLabels()
{
    return osgcolorbar_->getNumLabels();
}


int SceneColTab::getMinLabelWidth()
{
    float nrlabels = sequence_.nrSegments()+1;
    if ( nrlabels < cMinLabels )
	nrlabels = cMinLabels;

    const float start = rg_.start_;
    const float stop =	mIsZero(rg_.width(false),mDefEps) ? 1 : rg_.stop_;
    const float interval = stop - start;
    const float dist = interval / ( nrlabels - 1 );
    const float lbl = start + dist;
    const BufferString lblstring( lbl );
    const int lblwidth = lblstring.size()*
	    (osgcolorbar_->getTextProperties()._characterSize/mGoldenRatioF);
    const int totalwidth = lblwidth * ( horizontal_ ? nrlabels : 1 );
    return totalwidth;
}


int SceneColTab::getMinLabelHeight()
{
    const int numlabels = sequence_.hasEqualSegments() ? sequence_.nrSegments()
						       : cMinLabels;
    return osgcolorbar_->getTextProperties()._characterSize *
						(horizontal_ ? 1 : numlabels);
}


void SceneColTab::updateSequence()
{
    if ( !isOn() )
	return;

    const int nrcols = 256;
    ColTab::IndexedLookUpTable table( sequence_, nrcols );
    std::vector<osg::Vec4> colors(nrcols);

    mDefParallelCalc4Pars( ColorUpdator, tr("Color update"),
			   std::vector<osg::Vec4>&, colors,
			   const ColTab::IndexedLookUpTable&,table,
			   bool, flipseq,
			   const int&, nrcols )
    mDefParallelCalcBody
    (
	,
	OD::Color col = table_.colorForIndex( flipseq_ ? nrcols_-idx-1 : idx );
	colors_[idx] = osg::Vec4( col2f(r), col2f(g), col2f(b), 1-col2f(t) )
	,
    )

    ColorUpdator colorupdator( nrcols, colors, table, flipseq_, nrcols );
    colorupdator.execute();

    auto* osgcolorrange = new osgSim::ColorRange( rg_.start_,
					   mIsZero(rg_.width(false),mDefEps) ? 1
							  : rg_.stop_, colors );

    osgcolorbar_->setScalarsToColors( osgcolorrange );

    if ( horizontal_ )
	setSize( width_, height_ );
    else
	setSize( height_, width_ );

    const int segments = sequence_.nrSegments();
    setNumLabels( segments+1 );
    requestSingleRedraw();
}


void SceneColTab::setColTabMapperSetup( const ColTab::MapperSetup& ms )
{
    const Interval<float> rg = ms.range_;
    if ( rg.isUdf() || (rg==rg_ && flipseq_==ms.flipseq_) )
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
