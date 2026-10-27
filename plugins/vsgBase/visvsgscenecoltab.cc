/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgscenecoltab.h"

#include "visvsgdataman.h"
#include "visvsgfontcreator.h"
#include "vsggeoscalarbar.h"

#include "bufstring.h"
#include "coltabindex.h"
#include "coltabmapper.h"
#include "math2.h"
#include "typeset.h"

mCreateDGBVisFactoryEntry( dGBVis::SceneColTab );

namespace
{

constexpr int cMinLabels = 5;
constexpr int cMargin = 10;
constexpr int cTopMargin = 35;
constexpr float cGoldenRatio = 1.6180339887f;

using BarOrientation = vsgGeo::ScalarBar::Orientation;


} // anonymous namespace


namespace dGBVis
{

SceneColTab::SceneColTab()
    : bar_(vsgGeo::ScalarBar::create())
    , pixeldensity_(getDefaultPixelDensity())
{
    ref();
    mAttachCB( FontCreator::instance().fontLoaded, SceneColTab::fontLoadedCB );
    unRefNoDelete();
    setVsgNode( bar_.get() );
    setAnnotFont( FontData() );
    setColTabSequence( ColTab::Sequence("") );
    setSize( length_, thickness_ );
}


SceneColTab::~SceneColTab()
{
    detachAllNotifiers();
}


void SceneColTab::prepare()
{
    bar_->setUpdater( DataObject::getUpdater() );
}


void SceneColTab::setWindowSize( int winx, int winy )
{
    winx_ = winx;
    winy_ = winy;
    placeBar();
}


void SceneColTab::setPos( Pos pos )
{
    pos_ = pos;
    horizontal_ = pos_==Top || pos_==Bottom;
    applySize();
}


void SceneColTab::placeBar()
{
    const float length = float( length_ );
    const float thickness = float( thickness_ );
    const float winx = float( winx_ );
    const float winy = float( winy_ );
    float xpos = winx/2.f - length/2.f;
    float ypos = float( cMargin );
    if ( pos_ == Left )
    {
	xpos = float( cMargin );
	ypos = winy/2.f - length/2.f;
    }
    else if ( pos_ == Right )
    {
	xpos = winx - ( thickness*1.5f + float(getMinLabelWidth()) );
	ypos = winy/2.f - length/2.f;
    }
    else if ( pos_ == Top )
	ypos = winy - ( thickness + float(cTopMargin) );

    bar_->setOrientation( horizontal_ ? BarOrientation::Horizontal
				      : BarOrientation::Vertical );
    bar_->setPosition( vsg::vec2(xpos,ypos) );
    requestSingleRedraw();
}


void SceneColTab::setSize( int width, int height )
{
    const Interval<int> lengthrg = horizontal_
	? Interval<int>( ColorBarBounds::minHorWidth(),
			 ColorBarBounds::maxHorWidth() )
	: Interval<int>( ColorBarBounds::minVertHeight(),
			 ColorBarBounds::maxVertHeight() );
    const Interval<int> thicknessrg = horizontal_
	? Interval<int>( ColorBarBounds::minHorHeight(),
			 ColorBarBounds::maxHorHeight() )
	: Interval<int>( ColorBarBounds::minVertWidth(),
			 ColorBarBounds::maxVertWidth() );
    length_ = lengthrg.limitValue( horizontal_ ? width : height );
    thickness_ = thicknessrg.limitValue( horizontal_ ? height : width );
    bar_->setSize( float(length_), float(thickness_) );
    placeBar();
    setNumLabels( sequence_.nrSegments()+1 );
}


void SceneColTab::applySize()
{
    setSize( horizontal_ ? length_ : thickness_,
	     horizontal_ ? thickness_ : length_ );
}


Geom::Size2D<int> SceneColTab::getSize() const
{
    return Geom::Size2D<int>( horizontal_ ? length_ : thickness_,
			      horizontal_ ? thickness_ : length_ );
}


void SceneColTab::setAnnotFont( const FontData& fd )
{
    fontdata_ = fd;
    FontCreator& creator = FontCreator::instance();
    vsg::ref_ptr<vsg::Font> font = creator.get( fd );
    prepare();
    if ( font )
	bar_->setAnnotationFont( font );
    else
	creator.request( fd );

    const float sizefactor = pixeldensity_ / getDefaultPixelDensity();
    bar_->setAnnotationTextSize( fd.pointSize()*sizefactor );
    applySize();
    requestSingleRedraw();
}


void SceneColTab::fontLoadedCB( CallBacker* )
{
    vsg::ref_ptr<vsg::Font> font = FontCreator::instance().get( fontdata_ );
    if ( !font )
	return;

    prepare();
    bar_->setAnnotationFont( font );
    requestSingleRedraw();
}


void SceneColTab::setPixelDensity( float dpi )
{
    if ( dpi == pixeldensity_ )
	return;

    DataObject::setPixelDensity( dpi );
    pixeldensity_ = dpi;
    setAnnotFont( fontdata_ );
}


void SceneColTab::setLegendColor( const OD::Color& col )
{
    bar_->setAnnotationColor( toVec4(col,1.f) );
    requestSingleRedraw();
}


void SceneColTab::setColTabSequence( const ColTab::Sequence& ctseq )
{
    if ( sequence_ == ctseq )
	return;

    sequence_ = ctseq;
    updateSequence();
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
    const bool res = DataObject::turnOn( yn );
    updateSequence();
    return res;
}


void SceneColTab::updateSequence()
{
    if ( !isOn() )
	return;

    prepare();
    const int nrcols = vsgGeo::ScalarBar::cNrColors;
    ColTab::IndexedLookUpTable table( sequence_, nrcols );
    TypeSet<vsg::vec4> colors( nrcols, vsg::vec4() );
    for ( int idx=0; idx<nrcols; idx++ )
    {
	const OD::Color col = table.colorForIndex(
				    flipseq_ ? nrcols-idx-1 : idx );
	colors[idx] = toVec4( col );
    }

    bar_->setColors( colors.arr(), nrcols );
    bar_->setRange( rg_.start_,
		    mIsZero(rg_.width(false),mDefEps) ? 1.f : rg_.stop_ );
    applySize();
    requestSingleRedraw();
}


void SceneColTab::setNumLabels( int num )
{
    const int minlabels = sequence_.hasEqualSegments()
				? sequence_.nrSegments()+1 : cMinLabels;
    const int minsize = horizontal_ ? getMinLabelWidth() : getMinLabelHeight();
    if ( minsize <= length_ )
	bar_->setNumLabels( mMAX(num,minlabels) );
    else
	bar_->setNumLabels( cMinLabels );
}


int SceneColTab::getNumLabels() const
{
    return bar_->getNumLabels();
}


int SceneColTab::getLabelCharSize() const
{
    return int( bar_->getAnnotationTextSize() );
}


int SceneColTab::getMinLabelWidth() const
{
    float nrlabels = float( sequence_.nrSegments()+1 );
    if ( nrlabels < cMinLabels )
	nrlabels = cMinLabels;

    const float start = rg_.start_;
    const float stop = mIsZero(rg_.width(false),mDefEps) ? 1.f : rg_.stop_;
    const float dist = ( stop-start ) / ( nrlabels-1.f );
    const BufferString lblstring( start+dist );
    const int lblwidth = int( lblstring.size() *
			      (bar_->getAnnotationTextSize()/cGoldenRatio) );
    return lblwidth * ( horizontal_ ? int(nrlabels) : 1 );
}


int SceneColTab::getMinLabelHeight() const
{
    const int nrlabels = sequence_.hasEqualSegments()
				? sequence_.nrSegments() : cMinLabels;
    const int charsize = int( bar_->getAnnotationTextSize() );
    return charsize * ( horizontal_ ? 1 : nrlabels );
}

} // namespace dGBVis
