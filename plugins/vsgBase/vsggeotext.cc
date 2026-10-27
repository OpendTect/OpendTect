/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeotext.h"

#include <vsg/app/RecordTraversal.h>
#include <vsg/app/Viewer.h>
#include <vsg/maths/transform.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/text/GpuLayoutTechnique.h>
#include <vsg/vk/State.h>

#include <atomic>
#include <cmath>

namespace vsgGeo
{

namespace
{

class TextUpdate : public vsg::Inherit<Updatable,TextUpdate>
{
public:
		TextUpdate( Text& text )
		    : text_(&text)
		{}

    void	applyUpdates( vsg::Viewer& viewer, Compiler& compiler ) override
    {
	vsg::ref_ptr<Text> text = text_.ref_ptr();
	if ( text )
	    text->applyUpdates( viewer, compiler );
    }

private:

    vsg::observer_ptr<Text>	text_;
};

std::atomic<float>& viewportHeight()
{
    static std::atomic<float> height( 0.f );
    return height;
}


std::atomic<float>& horizontalScale()
{
    static std::atomic<float> scale( 1.f );
    return scale;
}

} // anonymous namespace


Text::Text()
{}


Text::~Text()
{}


void Text::setUpdater( Updater* updater )
{
    if ( updater_.ref_ptr().get() == updater )
	return;

    updater_ = updater;
    requestUpdate();
}


void Text::setOptions( vsg::ref_ptr<const vsg::Options> options )
{
    options_ = options;
}


void Text::setFont( vsg::ref_ptr<vsg::Font> font )
{
    if ( font == font_ )
	return;

    font_ = font;
    needsrebuild_ = true;
    requestUpdate();
}


void Text::setText( const std::u32string& text )
{
    if ( text == text_ )
	return;

    text_ = text;
    requestUpdate();
}


void Text::setPosition( const vsg::vec3& pos )
{
    if ( pos == position_ )
	return;

    position_ = pos;
    requestUpdate();
}


void Text::setCharacterHeight( float height )
{
    if ( height == height_ )
	return;

    height_ = height;
    requestUpdate();
}


void Text::setColor( const vsg::vec4& color )
{
    if ( color == color_ )
	return;

    color_ = color;
    requestUpdate();
}


void Text::setOutline( const vsg::vec4& color, float width )
{
    outlinecolor_ = color;
    outlinewidth_ = width;
    requestUpdate();
}


void Text::setAlignment( HAlign halign, VAlign valign )
{
    if ( halign==halign_ && valign==valign_ )
	return;

    halign_ = halign;
    valign_ = valign;
    requestUpdate();
}


void Text::setAxisAlignment( AxisAlignment axisalign )
{
    if ( axisalign == axisalign_ )
	return;

    axisalign_ = axisalign;
    requestUpdate();
}


void Text::setRotation( float radangle, const vsg::vec3& axis )
{
    const float len = vsg::length( axis );
    rotation_ = len>0.f ? vsg::rotate( radangle, axis/len ) : vsg::mat4();
    requestUpdate();
}


void Text::setAutoScaleDistance( float dist )
{
    if ( dist == autoscaledist_ )
	return;

    autoscaledist_ = dist;
    requestUpdate();
}


void Text::setScreenSized( bool yn )
{
    if ( yn == screensized_ )
	return;

    screensized_ = yn;
    screenworldheight_ = 0.f;
    requestUpdate();
}


void Text::setViewportHeight( float height )
{
    viewportHeight() = height;
}


void Text::setHorizontalScale( float scale )
{
    horizontalScale() = scale;
}


void Text::requestUpdate()
{
    vsg::ref_ptr<Updater> updater = updater_.ref_ptr();
    if ( !updater )
    {
	applyChanges( nullptr );
	return;
    }

    if ( !updatehelper_ )
	updatehelper_ = TextUpdate::create( *this );

    updater->requestUpdate( updatehelper_ );
}


void Text::fillLayout( vsg::StandardLayout& layout ) const
{
    vsg::vec3 hdir( 1.f, 0.f, 0.f );
    vsg::vec3 vdir( 0.f, 1.f, 0.f );
    switch ( axisalign_ )
    {
	case AxisAlignment::ReversedXY:
	    hdir.set( -1.f, 0.f, 0.f );
	    break;
	case AxisAlignment::XZ:
	    vdir.set( 0.f, 0.f, 1.f );
	    break;
	case AxisAlignment::ReversedXZ:
	    hdir.set( -1.f, 0.f, 0.f );
	    vdir.set( 0.f, 0.f, 1.f );
	    break;
	case AxisAlignment::YZ:
	    hdir.set( 0.f, 1.f, 0.f );
	    vdir.set( 0.f, 0.f, 1.f );
	    break;
	case AxisAlignment::ReversedYZ:
	    hdir.set( 0.f, -1.f, 0.f );
	    vdir.set( 0.f, 0.f, 1.f );
	    break;
	case AxisAlignment::User:
	{
	    const vsg::vec4 hrot = rotation_ * vsg::vec4( 1.f, 0.f, 0.f, 0.f );
	    const vsg::vec4 vrot = rotation_ * vsg::vec4( 0.f, 1.f, 0.f, 0.f );
	    hdir.set( hrot.x, hrot.y, hrot.z );
	    vdir.set( vrot.x, vrot.y, vrot.z );
	    break;
	}
	case AxisAlignment::XY:
	case AxisAlignment::Screen:
	    break;
    }

    const bool screenheight = screensized_ &&
			      axisalign_ == AxisAlignment::Screen;
    const float height = screenheight ? screenworldheight_ : height_;
    layout.position = position_;
    layout.horizontal = hdir * ( height * (screenheight ? screenhscale_ : 1.f) );
    layout.vertical = vdir * height;
    layout.color = color_;
    layout.outlineColor = outlinecolor_;
    layout.outlineWidth = outlinewidth_;
    layout.billboard = axisalign_ == AxisAlignment::Screen;
    layout.billboardAutoScaleDistance = autoscaledist_>0.f ? autoscaledist_
							     : -1e30f;

    layout.horizontalAlignment =
	halign_==HAlign::Left ? vsg::StandardLayout::LEFT_ALIGNMENT
	: halign_==HAlign::Center ? vsg::StandardLayout::CENTER_ALIGNMENT
				  : vsg::StandardLayout::RIGHT_ALIGNMENT;
    layout.verticalAlignment =
	valign_==VAlign::Bottom ? vsg::StandardLayout::BOTTOM_ALIGNMENT
	: valign_==VAlign::Center ? vsg::StandardLayout::CENTER_ALIGNMENT
				  : vsg::StandardLayout::TOP_ALIGNMENT;
}


vsg::ref_ptr<vsg::Data> Text::createTextData() const
{
    vsg::ref_ptr<vsg::uintArray> data =
			vsg::uintArray::create( uint32_t(text_.size()) );
    for ( size_t idx=0; idx<text_.size(); idx++ )
	data->at( idx ) = uint32_t( text_[idx] );

    return data;
}


vsg::ref_ptr<const vsg::Options> Text::effectiveOptions() const
{
    if ( options_ )
	return options_;

    vsg::ref_ptr<Updater> updater = updater_.ref_ptr();
    if ( updater )
	return updater->sharedOptions();

    return nullptr;
}


vsg::ref_ptr<vsg::ShaderSet> Text::textShaderSet()
{
    static const char* sShaderSetKey = "vsgGeo.text";
    vsg::ref_ptr<Updater> updater = updater_.ref_ptr();
    vsg::ref_ptr<vsg::Options> shared;
    if ( updater )
	shared = updater->sharedOptions();

    if ( shared )
    {
	const std::map<std::string,vsg::ref_ptr<vsg::ShaderSet>>::iterator itr =
				shared->shaderSets.find( sShaderSetKey );
	if ( itr != shared->shaderSets.end() )
	    shaderset_ = itr->second;
    }

    if ( !shaderset_ )
    {
	shaderset_ = vsg::createTextShaderSet( effectiveOptions() );
	for ( auto& state : shaderset_->defaultGraphicsPipelineStates )
	{
	    vsg::ref_ptr<vsg::ColorBlendState> blendstate =
					state.cast<vsg::ColorBlendState>();
	    if ( blendstate )
		blendstate->configureAttachments( true );
	}

	if ( shared )
	    shared->shaderSets[sShaderSetKey] = shaderset_;
    }

    return shaderset_;
}


void Text::applyUpdates( vsg::Viewer&, Compiler& compiler )
{
    applyChanges( &compiler );
}


void Text::applyChanges( Compiler* compiler )
{
    if ( !font_ )
	return;

    const bool billboard = axisalign_ == AxisAlignment::Screen;
    const bool mustrebuild = !current_ || needsrebuild_ ||
			     text_.size()>=capacity_ ||
			     billboard!=builtbillboard_;
    if ( mustrebuild )
	rebuild( compiler, billboard );
    else
	updateInPlace();

    needsrebuild_ = false;
}


void Text::rebuild( Compiler* compiler, bool billboard )
{
    vsg::ref_ptr<vsg::StandardLayout> layout = vsg::StandardLayout::create();
    fillLayout( *layout );

    uint32_t capacity = sMinCapacity;
    while ( capacity <= text_.size() )
	capacity *= 2;

    vsg::ref_ptr<vsg::Text> newtext = vsg::Text::create();
    newtext->font = font_;
    newtext->shaderSet = textShaderSet();
    newtext->layout = layout;
    newtext->text = createTextData();
    newtext->technique = vsg::GpuLayoutTechnique::create();
    newtext->setup( capacity, effectiveOptions() );

    capacity_ = capacity;
    vsg::ref_ptr<vsg::GpuLayoutTechnique> gputechnique =
			newtext->technique.cast<vsg::GpuLayoutTechnique>();
    if ( gputechnique && gputechnique->textArray )
	capacity_ = uint32_t( gputechnique->textArray->valueCount() );

    vsg::ref_ptr<vsg::Text> oldtext = current_;
    current_ = newtext;
    builtbillboard_ = billboard;
    children.clear();
    children.push_back( current_ );

    if ( !compiler )
	return;

    compiler->add( current_ );
    if ( oldtext )
	compiler->release( oldtext );
}


void Text::accept( vsg::RecordTraversal& rt ) const
{
    if ( screensized_ && current_ && builtbillboard_ )
	const_cast<Text*>( this )->updateScreenSize( *rt.getState() );

    vsg::Group::accept( rt );
}


void Text::updateScreenSize( const vsg::State& state )
{
    const float viewheight = viewportHeight().load();
    const vsg::dmat4& proj = state.projectionMatrixStack.top();
    const double projscale = std::abs( proj[1][1] );
    if ( viewheight<=0.f || projscale<=0. )
	return;

    double worldperpixel = 2. / ( projscale * viewheight );
    const bool isperspective = std::abs( proj[3][3] ) < 0.5;
    if ( isperspective )
    {
	const vsg::dvec3 eyepos = state.modelviewMatrixStack.top() *
	    vsg::dvec3( position_.x, position_.y, position_.z );
	const double distance = -eyepos.z;
	if ( distance<=0. )
	    return;

	worldperpixel *= distance;
    }

    const float newheight = float( height_ * worldperpixel );
    const float newhscale = horizontalScale().load();
    if ( std::abs(newheight-screenworldheight_) <= 1e-3f*screenworldheight_ &&
	 std::abs(newhscale-screenhscale_) <= 1e-3f )
	return;

    screenworldheight_ = newheight;
    screenhscale_ = newhscale;
    updateInPlace();
}


void Text::updateInPlace()
{
    vsg::ref_ptr<vsg::StandardLayout> layout =
			current_->layout.cast<vsg::StandardLayout>();
    if ( layout )
	fillLayout( *layout );

    current_->text = createTextData();
    current_->setup( 0, effectiveOptions() );
}

} // namespace vsgGeo
