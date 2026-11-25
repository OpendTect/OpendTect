/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoscalarbar.h"

#include "vsggeotriangles.h"

#include <algorithm>
#include <sstream>

namespace vsgGeo
{

namespace
{

constexpr int cNrQuadVertices = 6;
constexpr int cNrBorderQuads = 4;
constexpr int cFirstBorderVertex = ScalarBar::cNrColors * cNrQuadVertices;
constexpr int cFirstTickVertex = cFirstBorderVertex +
				 cNrBorderQuads*cNrQuadVertices;
constexpr int cNrVertices = cFirstTickVertex +
			    ScalarBar::cMaxLabels*cNrQuadVertices;
constexpr float cBarDepth = 0.f;
constexpr float cLineDepth = 0.5f;
constexpr float cLabelDepth = 1.f;
constexpr float cLineWidth = 1.f;
constexpr float cLabelHeightFactor = 1.5f;
constexpr float cVerticalLabelGap = 2.f;


void setQuad( vsg::vec3Array& vertices, int idx, float xmin, float ymin,
	      float xmax, float ymax, float zval )
{
    const vsg::vec3 pa( xmin, ymin, zval );
    const vsg::vec3 pb( xmax, ymin, zval );
    const vsg::vec3 pc( xmax, ymax, zval );
    const vsg::vec3 pd( xmin, ymax, zval );
    vertices.at( idx ) = pa;
    vertices.at( idx+1 ) = pb;
    vertices.at( idx+2 ) = pc;
    vertices.at( idx+3 ) = pa;
    vertices.at( idx+4 ) = pc;
    vertices.at( idx+5 ) = pd;
}


void setQuadColor( vsg::vec4Array& colors, int idx, const vsg::vec4& color )
{
    for ( int vidx=0; vidx<cNrQuadVertices; vidx++ )
	colors.at( idx+vidx ) = color;
}


std::string defaultFormat( float value )
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

} // anonymous namespace


ScalarBar::ScalarBar()
    : vertices_(vsg::vec3Array::create(cNrVertices))
    , vertexcolors_(vsg::vec4Array::create(cNrVertices))
    , colors_(cNrColors)
    , formatter_(defaultFormat)
{
    vertices_->properties.dataVariance = vsg::DYNAMIC_DATA;
    vertexcolors_->properties.dataVariance = vsg::DYNAMIC_DATA;
    for ( int idx=0; idx<cNrColors; idx++ )
    {
	const float level = float(idx) / float(cNrColors-1);
	colors_[idx] = vsg::vec4( level, level, level, 1.f );
    }

    updateGeometry();
    updateColors();
    vsg::ref_ptr<vsg::Node> node =
		createTriangleList( vertices_, vertexcolors_, {}, true );
    if ( node )
	addChild( node );

    for ( int idx=0; idx<cMaxLabels; idx++ )
    {
	texts_[idx] = Text::create();
	texts_[idx]->setAxisAlignment( Text::AxisAlignment::Screen );
	texts_[idx]->setScreenSized( true );
	texts_[idx]->setCharacterHeight( textsize_ );
	texts_[idx]->setAlignment( Text::HAlign::Center, Text::VAlign::Bottom );
	texts_[idx]->setColor( annotcolor_ );
	addChild( texts_[idx] );
    }

    updateLabels();
}


ScalarBar::~ScalarBar()
{}


void ScalarBar::setUpdater( Updater* updater )
{
    for ( auto& text : texts_ )
	text->setUpdater( updater );
}


void ScalarBar::setColors( const vsg::vec4* colors, int nrcolors )
{
    if ( !colors || nrcolors<1 )
	return;

    for ( int idx=0; idx<cNrColors; idx++ )
	colors_[idx] = colors[ std::min(nrcolors-1,idx*nrcolors/cNrColors) ];

    updateColors();
}


const vsg::vec4& ScalarBar::color( int idx ) const
{
    return colors_[ std::max(0,std::min(cNrColors-1,idx)) ];
}


void ScalarBar::setRange( float min, float max )
{
    min_ = min;
    max_ = max;
    updateLabels();
}


void ScalarBar::setNumLabels( int nrlabels )
{
    nrlabels_ = std::max( 2, std::min(cMaxLabels,nrlabels) );
    updateGeometry();
    updateLabels();
}


const std::u32string& ScalarBar::labelText( int idx ) const
{
    return texts_[ std::max(0,std::min(cMaxLabels-1,idx)) ]->text();
}


void ScalarBar::setLabelFormatter(
			const std::function<std::string(float)>& formatter )
{
    formatter_ = formatter ? formatter : defaultFormat;
    updateLabels();
}


void ScalarBar::setSize( float length, float thickness )
{
    length_ = length;
    thickness_ = thickness;
    updateGeometry();
    updateLabels();
}


void ScalarBar::setPosition( const vsg::vec2& pos )
{
    pos_ = pos;
    updateGeometry();
    updateLabels();
}


void ScalarBar::setOrientation( Orientation orientation )
{
    if ( orientation == orientation_ )
	return;

    orientation_ = orientation;
    const bool horizontal = orientation_ == Orientation::Horizontal;
    for ( auto& text : texts_ )
	text->setAlignment(
		horizontal ? Text::HAlign::Center : Text::HAlign::Left,
		horizontal ? Text::VAlign::Bottom : Text::VAlign::Center );

    updateGeometry();
    updateLabels();
}


void ScalarBar::setAnnotationColor( const vsg::vec4& color )
{
    annotcolor_ = color;
    updateColors();
    for ( auto& text : texts_ )
	text->setColor( color );
}


void ScalarBar::setAnnotationFont( vsg::ref_ptr<vsg::Font> font )
{
    for ( auto& text : texts_ )
	text->setFont( font );
}


void ScalarBar::setAnnotationTextSize( float size )
{
    textsize_ = size;
    for ( auto& text : texts_ )
	text->setCharacterHeight( size );
}


void ScalarBar::updateGeometry()
{
    const bool horizontal = orientation_ == Orientation::Horizontal;
    const float left = pos_.x;
    const float bottom = pos_.y;
    const float right = left + ( horizontal ? length_ : thickness_ );
    const float top = bottom + ( horizontal ? thickness_ : length_ );
    const float step = length_ / float( cNrColors );
    for ( int idx=0; idx<cNrColors; idx++ )
    {
	if ( horizontal )
	    setQuad( *vertices_, idx*cNrQuadVertices, left+step*idx,
		     bottom, left+step*(idx+1), top, cBarDepth );
	else
	    setQuad( *vertices_, idx*cNrQuadVertices, left,
		     bottom+step*idx, right, bottom+step*(idx+1), cBarDepth );
    }

    setQuad( *vertices_, cFirstBorderVertex, left, bottom,
	     left+cLineWidth, top, cLineDepth );
    setQuad( *vertices_, cFirstBorderVertex+cNrQuadVertices, right-cLineWidth,
	     bottom, right, top, cLineDepth );
    setQuad( *vertices_, cFirstBorderVertex+2*cNrQuadVertices, left, bottom,
	     right, bottom+cLineWidth, cLineDepth );
    setQuad( *vertices_, cFirstBorderVertex+3*cNrQuadVertices, left,
	     top-cLineWidth, right, top, cLineDepth );

    const float labelstep = length_ / float( nrlabels_-1 );
    for ( int idx=0; idx<cMaxLabels; idx++ )
    {
	const int vidx = cFirstTickVertex + idx*cNrQuadVertices;
	if ( idx >= nrlabels_ )
	{
	    setQuad( *vertices_, vidx, left, bottom, left, bottom, cLineDepth );
	    continue;
	}

	if ( horizontal )
	{
	    const float x = left + labelstep*idx;
	    setQuad( *vertices_, vidx, x-cLineWidth/2.f, top, x+cLineWidth/2.f,
		     bottom+thickness_*cLabelHeightFactor, cLineDepth );
	}
	else
	{
	    const float y = bottom + labelstep*idx;
	    setQuad( *vertices_, vidx, right, y-cLineWidth/2.f,
		     left+thickness_*cLabelHeightFactor, y+cLineWidth/2.f,
		     cLineDepth );
	}
    }

    vertices_->dirty();
}


void ScalarBar::updateColors()
{
    for ( int idx=0; idx<cNrColors; idx++ )
	setQuadColor( *vertexcolors_, idx*cNrQuadVertices, colors_[idx] );

    for ( int idx=0; idx<cNrBorderQuads+cMaxLabels; idx++ )
	setQuadColor( *vertexcolors_,
		      cFirstBorderVertex + idx*cNrQuadVertices, annotcolor_ );

    vertexcolors_->dirty();
}


void ScalarBar::updateLabels()
{
    const float labelstep = length_ / float( nrlabels_-1 );
    const float valuestep = ( max_-min_ ) / float( nrlabels_-1 );
    const bool horizontal = orientation_ == Orientation::Horizontal;
    const float labeloffset = thickness_*cLabelHeightFactor;
    for ( int idx=0; idx<cMaxLabels; idx++ )
    {
	if ( idx >= nrlabels_ )
	{
	    texts_[idx]->setText( std::u32string() );
	    continue;
	}

	const std::string label = formatter_( min_ + valuestep*idx );
	texts_[idx]->setText( std::u32string(label.begin(),label.end()) );
	const float xpos = horizontal
			 ? pos_.x + labelstep*idx
			 : pos_.x + labeloffset + cVerticalLabelGap;
	const float ypos = horizontal
			 ? pos_.y + labeloffset
			 : pos_.y + labelstep*idx;
	texts_[idx]->setPosition( vsg::vec3(xpos,ypos,cLabelDepth) );
    }
}

} // namespace vsgGeo
