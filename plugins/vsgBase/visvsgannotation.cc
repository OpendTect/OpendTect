/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgannotation.h"

#include "visvsgdataman.h"
#include "visvsgtransform.h"
#include "visvsgtext.h"
#include "vsggeolines.h"
#include "vsggeoonesiderender.h"

#include "axislayout.h"
#include "iopar.h"
#include "ranges.h"
#include "samplingdata.h"

#include <vsg/nodes/Group.h>

static constexpr int cNrCorners = 8;
static constexpr int cNrGridSides = 6;

mCreateDGBVisFactoryEntry( dGBVis::Annotation );

namespace dGBVis
{

const char* Annotation::textprefixstr()		{ return "Text "; }
const char* Annotation::cornerprefixstr()	{ return "Corner "; }
const char* Annotation::showtextstr()		{ return "Show Text"; }
const char* Annotation::showscalestr()		{ return "Show Scale"; }

static const int dim0psindexes[]		= { 2, 5, 3, 4 };
static const int dim0coordindexes[]		= { 0, 4, 7, 3 };

static const int dim1psindexes[]		= { 0, 5, 1, 4 };
static const int dim1coordindexes[]		= { 0, 4, 5, 1 };

static const int dim2psindexes[]		= { 0, 3, 1, 2 };
static const int dim2coordindexes[]		= { 0, 3, 2, 1 };

static const int* psindexarr[] = {dim0psindexes,dim1psindexes,dim2psindexes};
static const int* coordindexarr[] =
    { dim0coordindexes, dim1coordindexes, dim2coordindexes };

static const int boxindices[] = { 0, 1, 1, 2, 2, 3, 3, 0,
				  4, 5, 5, 6, 6, 7, 7, 4,
				  0, 4, 1, 5, 2, 6, 3, 7 };


static TrcKeyZSampling getNiceScale( const TrcKeyZSampling& tkzs )
{
    TrcKeyZSampling scale = tkzs;

    const AxisLayout<int> inlal( (Interval<int>)tkzs.hsamp_.inlRange() );
    scale.hsamp_.start_.inl() = inlal.sd_.start_;
    scale.hsamp_.step_.inl() = inlal.sd_.step_;

    const AxisLayout<int> crlal( (Interval<int>)tkzs.hsamp_.crlRange() );
    scale.hsamp_.start_.crl() = crlal.sd_.start_;
    scale.hsamp_.step_.crl() = crlal.sd_.step_;

    const AxisLayout<float> zal( (Interval<float>)tkzs.zsamp_ );
    scale.zsamp_.start_ = zal.sd_.start_; scale.zsamp_.step_ = zal.sd_.step_;

    return scale;
}


static SamplingData<float> getAxisSD( const TrcKeyZSampling& tkzs, int dim,
				      int* stopidx=nullptr )
{
    SamplingData<float> sd;
    float stop = mUdf( float );
    if ( dim==0 )
    {
	sd.set( AxisLayout<int>(tkzs.hsamp_.inlRange()).sd_ );
	stop = tkzs.hsamp_.inlRange().stop_;
    }
    else if ( dim==1 )
    {
	sd.set( AxisLayout<int>(tkzs.hsamp_.crlRange()).sd_ );
	stop = tkzs.hsamp_.crlRange().stop_;
    }
    else
    {
	sd.set( AxisLayout<float>(tkzs.zsamp_).sd_ );
	stop = tkzs.zsamp_.stop_;
    }

    const float eps = 1e-6;
    if ( stopidx )
	*stopidx = sd.step_==0.f
		     ? -1 : static_cast<int>( sd.getfIndex(stop+eps*sd.step_) );

    return sd;
}


Annotation::Annotation()
    : VisualObjectImpl(true)
    , tkzs_(false)
    , tkzsdefaultscale_(false)
    , scale_(false)
    , annotcolor_(OD::Color::White())
{
    ref();
    scalefactor_[0] = scalefactor_[1] = scalefactor_[2] = 1.;
    for ( int& nr : nrgridsegments_ )
	nr = 0;

    corners_[0] = vsg::vec3( 0, 0, 0 );
    corners_[1] = vsg::vec3( 0, 0, 1 );
    corners_[2] = vsg::vec3( 0, 1, 1 );
    corners_[3] = vsg::vec3( 0, 1, 0 );
    corners_[4] = vsg::vec3( 1, 0, 0 );
    corners_[5] = vsg::vec3( 1, 0, 1 );
    corners_[6] = vsg::vec3( 1, 1, 1 );
    corners_[7] = vsg::vec3( 1, 1, 0 );

    boxcoords_ = vsg::vec3Array::create( 24 );
    boxcoords_->properties.dataVariance = vsg::DYNAMIC_DATA;
    boxcolor_ = vsgGeo::createLineColor( toVec4(annotcolor_) );
    updateBox();
    boxnode_ = vsgGeo::createLineList( boxcoords_, boxcolor_ );
    addChild( boxnode_.get() );

    axisnametexts_ = Text2::create();
    ticktexts_ = Text2::create();
    for ( int dim=0; dim<3; dim++ )
    {
	Text* text = axisnametexts_->text( axisnametexts_->addText() );
	text->setJustification( Text::Right );
	text->setColor( annotcolor_ );
    }

    if ( textshown_ )
	addChild( axisnametexts_->vsgNode() );
    if ( scaleshown_ )
	addChild( ticktexts_->vsgNode() );

    updateGridLines();
    setPickable( true, true );
    unRefNoDelete();
}


Annotation::~Annotation()
{
    detachAllNotifiers();
}


void Annotation::showText( bool yn )
{
    if ( yn==textshown_ )
	return;

    textshown_ = yn;
    if ( yn )
	addChild( axisnametexts_->vsgNode() );
    else
	removeChild( axisnametexts_->vsgNode() );
}


void Annotation::showScale( bool yn )
{
    if ( yn==scaleshown_ )
	return;

    scaleshown_ = yn;
    if ( yn )
	addChild( ticktexts_->vsgNode() );
    else
	removeChild( ticktexts_->vsgNode() );
}


void Annotation::showGridLines( bool yn )
{
    if ( yn==gridshown_ )
	return;

    gridshown_ = yn;
    if ( !gridlines_ )
	return;

    if ( yn )
	addChild( gridlines_.get() );
    else
	removeChild( gridlines_.get() );
}


void Annotation::setDisplayTransformation( const mDGBVisTrans* tr )
{
    displaytrans_ = tr;
    setTrcKeyZSampling( tkzs_ );
}


const mDGBVisTrans* Annotation::getDisplayTransformation() const
{
    return displaytrans_.ptr();
}


void Annotation::setTrcKeyZSampling( const TrcKeyZSampling& tkzs )
{
    tkzs_ = tkzs;
    if ( !tkzs_.isDefined() )
	return;

    tkzsdefaultscale_ = getNiceScale( tkzs_ );

    const Interval<int> inlrg = tkzs.hsamp_.inlRange();
    const Interval<int> crlrg = tkzs.hsamp_.crlRange();
    const Interval<float>& zrg = tkzs.zsamp_;

    setCorner( 0, inlrg.start_, crlrg.start_, zrg.start_ );
    setCorner( 1, inlrg.stop_, crlrg.start_, zrg.start_ );
    setCorner( 2, inlrg.stop_, crlrg.stop_, zrg.start_ );
    setCorner( 3, inlrg.start_, crlrg.stop_, zrg.start_ );
    setCorner( 4, inlrg.start_, crlrg.start_, zrg.stop_ );
    setCorner( 5, inlrg.stop_, crlrg.start_, zrg.stop_ );
    setCorner( 6, inlrg.stop_, crlrg.stop_, zrg.stop_ );
    setCorner( 7, inlrg.start_, crlrg.stop_, zrg.stop_ );

    updateBox();
    updateGridLines();
}


void Annotation::setScale( const TrcKeyZSampling& tkzs,
			   const double* scalefacs, int nrvals,
			   bool makescalenice )
{
    scale_ = makescalenice ? getNiceScale(tkzs) : tkzs;
    if ( scalefacs && *scalefacs && nrvals==3 )
    {
	for ( int idx=0; idx<3; idx++ )
	    scalefactor_[idx] = scalefacs[idx];
    }

    updateGridLines();
}


const TrcKeyZSampling& Annotation::getScale( bool getdefault ) const
{
    return getdefault ? tkzsdefaultscale_
		      : scale_.isEmpty() ? tkzsdefaultscale_ : scale_;
}


void Annotation::setText( int dim, const uiString& string )
{
    if ( dim<0 || dim>=3 )
	return;

    axisnames_[dim] = string;
    axisnametexts_->text( dim )->setText( string );
}


const uiString& Annotation::getText( int dim ) const
{
    return axisnames_[ dim>=0 && dim<3 ? dim : 0 ];
}


void Annotation::setColor( const OD::Color& col )
{
    annotcolor_ = col;
    const vsg::vec4 vsgcol = toVec4( col );
    boxcolor_->at( 0 ) = vsgcol;
    boxcolor_->dirty();
    for ( auto* gridcol : gridcolors_ )
    {
	gridcol->at( 0 ) = vsgcol;
	gridcol->dirty();
    }

    updateTextColor();
    requestSingleRedraw();
}


const FontData& Annotation::getFont() const
{
    return axisnametexts_->text()->getFontData();
}


void Annotation::setFont( const FontData& fd )
{
    axisnametexts_->setFontData( fd );
    ticktexts_->setFontData( fd );
}


void Annotation::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );
    axisnametexts_->setPixelDensity( dpi );
    ticktexts_->setPixelDensity( dpi );
}


Coord3 Annotation::getCorner( int idx ) const
{
    if ( idx<0 || idx>=cNrCorners )
	return Coord3::udf();

    const vsg::vec3& pos = corners_[idx];
    return Coord3( pos.x, pos.y, pos.z );
}


int Annotation::nrGridLineSegments( int side ) const
{
    return side>=0 && side<cNrGridSides ? nrgridsegments_[side] : 0;
}


void Annotation::setCorner( int idx, double x, double y, double z )
{
    Transformation::transform( displaytrans_.ptr(),
			       Coord3(float(x),float(y),float(z)),
			       corners_[idx] );
}


void Annotation::updateBox()
{
    for ( int idx=0; idx<24; idx++ )
	boxcoords_->at( idx ) = corners_[ boxindices[idx] ];

    boxcoords_->dirty();
    requestSingleRedraw();
}


void Annotation::getAxisCoords( int dim, vsg::vec3& p0, vsg::vec3& p1 ) const
{
    const int pidx0 = 0;
    const int pidx1 = dim==0 ? 1 : (dim==1 ? 3 : 4);
    Transformation::transformBack( displaytrans_.ptr(), corners_[pidx0], p0 );
    Transformation::transformBack( displaytrans_.ptr(), corners_[pidx1], p1 );
}


void Annotation::updateGridLines()
{
    TypeSet<vsg::vec3> sides[cNrGridSides];
    ticks_.erase();
    vsg::ref_ptr<vsgGeo::OneSideRender> newgrid =
				vsgGeo::OneSideRender::create();
    const bool hasrange = tkzs_.isDefined();
    for ( int dim=0; dim<3; dim++ )
    {
	const double currscalefact = scalefactor_[dim];
	vsg::vec3 p0, p1;
	getAxisCoords( dim, p0, p1 );

	vsg::vec3 dir( 0.f, 0.f, 0.f );
	dir[dim] = 1.f;
	Transformation::transformDir( displaytrans_.ptr(), dir );
	dir = vsg::normalize( dir );

	vsg::vec3 lstart0, lstart1;
	Transformation::transform( displaytrans_.ptr(), p0, lstart0 );
	Transformation::transform( displaytrans_.ptr(), p1, lstart1 );
	newgrid->setLine( dim*2, vsg::dvec3(lstart0), vsg::dvec3(dir) );
	newgrid->setLine( dim*2+1, vsg::dvec3(lstart1), -vsg::dvec3(dir) );

	if ( !hasrange )
	    continue;

	Interval<float> range( p0[dim], p1[dim] );
	range.scale( currscalefact );

	int stopidx;
	const SamplingData<float> sd = getAxisSD( getScale(false),
						  dim, &stopidx );

	const int* psindexes = psindexarr[dim];
	const int* cornerarr = coordindexarr[dim];
	vsg::vec3 corners[4];
	for ( int idy=0; idy<4; idy++ )
	    Transformation::transformBack( displaytrans_.ptr(),
					   corners_[cornerarr[idy]],
					   corners[idy] );

	for ( int idx=0; idx<=stopidx; idx++ )
	{
	    const float val = sd.atIndex( idx );
	    if ( val <= range.start_ )
		continue;
	    else if ( val > range.stop_ )
		break;

	    const float unscaledval = val / currscalefact;
	    vsg::vec3 pos[4];
	    for ( int idy=0; idy<4; idy++ )
	    {
		corners[idy][dim] = unscaledval;
		Transformation::transform( displaytrans_.ptr(), corners[idy],
					   pos[idy] );
	    }

	    for ( int idy=0; idy<4; idy++ )
	    {
		TypeSet<vsg::vec3>& side = sides[ psindexes[idy] ];
		side += pos[idy];
		side += pos[ idy!=3 ? idy+1 : 0 ];
	    }

	    vsg::vec3 tickpos( p0 );
	    tickpos[dim] = float( val/currscalefact );
	    Transformation::transform( displaytrans_.ptr(), tickpos );
	    ticks_ += Tick{ dim, val,
			    Coord3(tickpos.x,tickpos.y,tickpos.z) };
	}
    }

    gridcolors_.setEmpty();
    const vsg::vec4 col = toVec4( annotcolor_ );
    for ( int idx=0; idx<cNrGridSides; idx++ )
    {
	const TypeSet<vsg::vec3>& side = sides[idx];
	nrgridsegments_[idx] = side.size() / 2;
	vsg::ref_ptr<vsg::Node> sidenode;
	if ( !side.isEmpty() )
	{
	    vsg::ref_ptr<vsg::vec3Array> coords =
			vsg::vec3Array::create( od_uint32(side.size()) );
	    for ( int idy=0; idy<side.size(); idy++ )
		coords->at( idy ) = side[idy];

	    vsg::ref_ptr<vsg::vec4Array> sidecol =
					vsgGeo::createLineColor( col );
	    sidenode = vsgGeo::createLineList( coords, sidecol );
	    if ( sidenode )
		gridcolors_ += sidecol.get();
	}

	if ( !sidenode )
	    sidenode = vsg::Group::create();

	newgrid->addChild( sidenode );
    }

    if ( gridshown_ && gridlines_ )
	removeChild( gridlines_.get() );

    gridlines_ = newgrid;
    if ( gridshown_ )
	addChild( gridlines_.get() );

    updateTextPos();
}


void Annotation::updateTextPos()
{
    for ( int dim=0; dim<3; dim++ )
    {
	vsg::vec3 p0, p1;
	getAxisCoords( dim, p0, p1 );
	vsg::vec3 middle( (p0.x+p1.x)/2.f, (p0.y+p1.y)/2.f, (p0.z+p1.z)/2.f );
	Transformation::transform( displaytrans_.ptr(), middle );
	axisnametexts_->text( dim )->setPosition( middle );
    }

    const int nrticks = ticks_.size();
    while ( ticktexts_->nrTexts() < nrticks )
	ticktexts_->addText();
    while ( ticktexts_->nrTexts() > nrticks )
	ticktexts_->removeText( ticktexts_->text(ticktexts_->nrTexts()-1) );

    for ( int idx=0; idx<nrticks; idx++ )
    {
	const Tick& tick = ticks_[idx];
	Text* text = ticktexts_->text( idx );
	text->setPosition( vsg::vec3(float(tick.pos_.x_),float(tick.pos_.y_),
				     float(tick.pos_.z_)) );
	text->setText( toUiString(tick.value_) );
    }

    const FontData fd = getFont();
    axisnametexts_->setFontData( fd );
    ticktexts_->setFontData( fd );
    updateTextColor();
}


void Annotation::updateTextColor()
{
    for ( int idx=0; idx<axisnametexts_->nrTexts(); idx++ )
	axisnametexts_->text( idx )->setColor( annotcolor_ );

    for ( int idx=0; idx<ticktexts_->nrTexts(); idx++ )
	ticktexts_->text( idx )->setColor( annotcolor_ );
}


void Annotation::fillPar( IOPar& par ) const
{
    BufferString key;
    for ( int idx=0; idx<cNrCorners; idx++ )
    {
	key = cornerprefixstr();
	key += idx;
	const vsg::vec3& pos = corners_[idx];
	par.set( key, pos.x, pos.y, pos.z );
    }

    par.setYN( showtextstr(), isTextShown() );
    par.setYN( showscalestr(), isScaleShown() );
}


bool Annotation::usePar( const IOPar& par )
{
    BufferString key;
    vsg::vec3 corners[cNrCorners];
    for ( int idx=0; idx<cNrCorners; idx++ )
    {
	key = cornerprefixstr();
	key += idx;

	double x, y, z;
	if ( !par.get(key,x,y,z) )
	    return false;

	corners[idx].set( float(x), float(y), float(z) );
    }

    for ( int idx=0; idx<cNrCorners; idx++ )
	corners_[idx] = corners[idx];

    for ( int idx=0; idx<3; idx++ )
    {
	key = textprefixstr();
	key += idx;
	const BufferString text = par.find( key );
	if ( !text.isEmpty() )
	    setText( idx, toUiString(text) );
    }

    bool yn = true;
    par.getYN( showtextstr(), yn );
    showText( yn );

    yn = true;
    par.getYN( showscalestr(), yn );
    showScale( yn );

    updateBox();
    updateGridLines();
    return true;
}

} // namespace dGBVis
