/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeothumbwheel.h"

#include "vsggeotriangles.h"

#include <vsg/app/RecordTraversal.h>
#include <vsg/app/Window.h>

#include <algorithm>
#include <cmath>

namespace vsgGeo
{

namespace
{

constexpr float cPi = 3.14159265358979f;
constexpr int cNrSlices = 180;
constexpr int cNrQuadsPerSlice = 5;
constexpr int cNrOutlineQuads = 4;
constexpr int cNrQuadVertices = 6;
constexpr int cNrWheelVertices = cNrSlices * cNrQuadsPerSlice * cNrQuadVertices;
constexpr int cNrOutlineVertices = cNrOutlineQuads * cNrQuadVertices;
constexpr int cNrVertices = cNrWheelVertices + cNrOutlineVertices;
constexpr float cRadsPerTick = 20.f * cPi / 180.f;
constexpr float cMaxRotationSpeed = 5.f * cRadsPerTick;
constexpr float cTickCenter = 1.f / 16.f;
constexpr float cTickHalfWidth = 1.f / 10.f;
constexpr float cGlassTickHalfWidth = 1.f / 16.f;
constexpr float cGlassBandBase = 0.02f;
constexpr float cGlassBandGain = 0.12f;
constexpr float cGlassTickBase = 0.25f;
constexpr float cGlassTickGain = 0.65f;
constexpr float cInsetFraction = 0.14f;
constexpr float cOutlineWidth = 1.f;
constexpr float cOutlineLift = 0.5f;
constexpr float cGrowWidthFactor = 2.f;
constexpr float cGrowLengthFactor = 1.6f;
constexpr float cGrowTime = 0.15f;

using Clock = std::chrono::steady_clock;


vsg::vec3 wheelPoint( bool horizontal, float along, float across, float zval )
{
    return horizontal ? vsg::vec3( along, across, zval )
		      : vsg::vec3( across, along, zval );
}


void setQuad( vsg::vec3Array& vertices, int idx, const vsg::vec3& pa,
	      const vsg::vec3& pb, const vsg::vec3& pc, const vsg::vec3& pd )
{
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


float tickCoverage( float angle, float halfwidth )
{
    const float ticks = angle / cRadsPerTick;
    const float frac = ticks - std::floor( ticks );
    float dist = std::abs( frac - cTickCenter );
    dist = std::min( dist, 1.f - dist );
    return std::max( 0.f, 1.f - dist/halfwidth );
}

} // anonymous namespace


ThumbWheel::ThumbWheel()
    : vertices_(vsg::vec3Array::create(cNrVertices))
    , colors_(vsg::vec4Array::create(cNrVertices))
{
    vertices_->properties.dataVariance = vsg::DYNAMIC_DATA;
    colors_->properties.dataVariance = vsg::DYNAMIC_DATA;
    updateShape();
    updateColors();
    vsg::ref_ptr<vsg::Node> node =
		createTriangleList( vertices_, colors_, {}, true );
    if ( node )
	addChild( node );
}


ThumbWheel::~ThumbWheel()
{}


void ThumbWheel::setShape( bool horizontal, const vsg::vec2& min,
			   const vsg::vec2& max, float zval )
{
    tracking_ = false;
    horizontal_ = horizontal;
    min_ = min;
    max_ = max;
    zval_ = zval;
    updateShape();
}


void ThumbWheel::setAngle( float angle, float rotationtime )
{
    if ( std::abs(angle-angle_) < 1e-5f )
	return;

    angle_ = angle;
    fading_ = false;
    opacity_ = 1.f;
    if ( rotationtime > 0.f )
    {
	rotationleft_ = rotationtime;
	return;
    }

    rotationleft_ = 0.f;
    phase_ = angle_;
    startFade();
    updateColors();
}


void ThumbWheel::enableFadeInOut( bool yn )
{
    if ( yn == fadeinout_ )
	return;

    fadeinout_ = yn;
    if ( !fadeinout_ )
    {
	fading_ = false;
	opacity_ = 1.f;
    }
    else
	startFade();

    updateColors();
}


void ThumbWheel::setFadeTime( float seconds )
{
    fadetime_ = std::max( 0.f, seconds );
}


void ThumbWheel::setStyle( Style style )
{
    if ( style == style_ )
	return;

    style_ = style;
    updateColors();
}


void ThumbWheel::setGrowOnHover( bool yn )
{
    if ( yn == growonhover_ )
	return;

    growonhover_ = yn;
    if ( yn )
	return;

    growth_ = 0.f;
    updateShape();
}


void ThumbWheel::setBorderColor( const vsg::vec4& color )
{
    bordercolor_ = color;
    updateColors();
}


void ThumbWheel::setRotationCallback( const std::function<void(float)>& cb )
{
    rotationcb_ = cb;
}


void ThumbWheel::setGrowDirection( int dir )
{
    growdirection_ = dir<0 ? -1 : (dir>0 ? 1 : 0);
    updateShape();
}


void ThumbWheel::currentExtents( vsg::vec2& min, vsg::vec2& max ) const
{
    extentsAt( growth_, min, max );
}


void ThumbWheel::extentsAt( float growth, vsg::vec2& min, vsg::vec2& max ) const
{
    min = min_;
    max = max_;
    const int along = dim();
    const int across = 1 - along;
    const float acrosscenter = ( min_[across]+max_[across] ) / 2.f;
    const float acrosshalf = ( max_[across]-min_[across] ) / 2.f *
			     ( 1.f + (cGrowWidthFactor-1.f)*growth );
    min[across] = acrosscenter - acrosshalf;
    max[across] = acrosscenter + acrosshalf;

    const float length = max_[along] - min_[along];
    const float extra = length * (cGrowLengthFactor-1.f) * growth;
    if ( growdirection_ > 0 )
	max[along] += extra;
    else if ( growdirection_ < 0 )
	min[along] -= extra;
    else
    {
	min[along] -= extra / 2.f;
	max[along] += extra / 2.f;
    }
}


ThumbWheel::Proximity ThumbWheel::proximity( float x, float y ) const
{
    vsg::vec2 min;
    vsg::vec2 max;
    currentExtents( min, max );
    if ( x>min.x && x<max.x && y>min.y && y<max.y )
	return Proximity::Above;

    const vsg::vec2 center = ( min_ + max_ ) / 2.f;
    const vsg::vec2 diff = center - vsg::vec2( x, y );
    const vsg::vec2 hotarea = ( min_ - max_ ) * 2.f;
    if ( vsg::dot(diff,diff) < vsg::dot(hotarea,hotarea) )
	return Proximity::Nearby;

    return Proximity::None;
}


void ThumbWheel::setProximity( Proximity prox )
{
    if ( prox == proximity_ )
	return;

    proximity_ = prox;
    startFade();
}


void ThumbWheel::startFade()
{
    fadefrom_ = opacity_;
    fading_ = true;
    fadestartpending_ = true;
}


bool ThumbWheel::handlePointer( Pointer type, float x, float y )
{
    setProximity( proximity(x,y) );
    vsg::vec2 growmin;
    vsg::vec2 growmax;
    extentsAt( 1.f, growmin, growmax );
    ingrowarea_ = x>growmin.x && x<growmax.x && y>growmin.y && y<growmax.y;
    const float pos = horizontal_ ? x : y;
    if ( !tracking_ )
    {
	if ( type==Pointer::Press && proximity_==Proximity::Above )
	{
	    vsg::vec2 min;
	    vsg::vec2 max;
	    currentExtents( min, max );
	    tracking_ = true;
	    startpos_ = pos;
	    startangle_ = angle_;
	    startlength_ = max[dim()] - min[dim()];
	    return true;
	}

	return false;
    }

    if ( type == Pointer::Release )
    {
	tracking_ = false;
	return true;
    }

    const float newangle = startangle_ + (pos-startpos_)*2.f/startlength_;
    const float delta = angle_ - newangle;
    setAngle( newangle );
    if ( delta!=0.f && rotationcb_ )
	rotationcb_( delta );

    return true;
}


void ThumbWheel::accept( vsg::RecordTraversal& rt ) const
{
    const_cast<ThumbWheel*>( this )->advance( Clock::now() );
    vsg::Group::accept( rt );
}


void ThumbWheel::advance( Clock::time_point now )
{
    if ( !hasadvanced_ )
    {
	lastadvance_ = now;
	hasadvanced_ = true;
    }

    const float passed =
		std::chrono::duration<float>( now-lastadvance_ ).count();
    lastadvance_ = now;
    bool changed = false;
    if ( rotationleft_ > 0.f )
    {
	changed = true;
	if ( passed >= rotationleft_ )
	{
	    phase_ = angle_;
	    rotationleft_ = 0.f;
	    startFade();
	}
	else
	{
	    const float maxstep = cMaxRotationSpeed * passed;
	    const float step = (angle_-phase_) * passed / rotationleft_;
	    phase_ += std::max( -maxstep, std::min(maxstep,step) );
	    rotationleft_ -= passed;
	}

	opacity_ = 1.f;
    }
    else if ( !fadeinout_ )
    {
	if ( opacity_ != 1.f )
	{
	    opacity_ = 1.f;
	    changed = true;
	}
    }
    else if ( proximity_==Proximity::Above && !tracking_ )
    {
	if ( opacity_ != 1.f )
	{
	    opacity_ = 1.f;
	    changed = true;
	}

	fading_ = false;
    }
    else if ( fading_ )
    {
	if ( fadestartpending_ )
	{
	    fadestart_ = now;
	    fadestartpending_ = false;
	}

	const float elapsed =
		std::chrono::duration<float>( now-fadestart_ ).count();
	const float progress = fadetime_>0.f
				    ? std::min( 1.f, elapsed/fadetime_ ) : 1.f;
	const float target = proximity_==Proximity::None ? 0.f : 1.f;
	opacity_ = fadefrom_ + (target-fadefrom_)*progress;
	fading_ = progress < 1.f;
	changed = true;
    }

    const bool grown = growonhover_ && (tracking_ || ingrowarea_);
    const float growtarget = grown ? 1.f : 0.f;
    if ( growth_ != growtarget )
    {
	const float step = passed / cGrowTime;
	growth_ = growtarget>growth_ ? std::min( growtarget, growth_+step )
				     : std::max( growtarget, growth_-step );
	updateShape();
    }

    if ( changed )
	updateColors();
}


void ThumbWheel::updateShape()
{
    const int dimension = dim();
    const int other = 1 - dimension;
    vsg::vec2 min;
    vsg::vec2 max;
    currentExtents( min, max );
    const float radius = ( max[dimension]-min[dimension] ) / 2.f;
    const float center = ( max[dimension]+min[dimension] ) / 2.f;
    const float low = min[other];
    const float high = max[other];
    const float endwidth = ( max_[other]-min_[other] ) / 16.f;
    const float inset = std::max( endwidth, (high-low)*cInsetFraction );
    const float across[cNrQuadsPerSlice+1] =
	{ low, low+endwidth, low+inset, high-inset, high-endwidth, high };
    for ( int slice=0; slice<cNrSlices; slice++ )
    {
	const float angle0 = cPi * slice / cNrSlices;
	const float angle1 = cPi * (slice+1) / cNrSlices;
	const float along0 = center + radius*std::cos( angle0 );
	const float along1 = center + radius*std::cos( angle1 );
	const int base = slice * cNrQuadsPerSlice * cNrQuadVertices;
	for ( int quad=0; quad<cNrQuadsPerSlice; quad++ )
	    setQuad( *vertices_, base+quad*cNrQuadVertices,
		     wheelPoint(horizontal_,along0,across[quad],zval_),
		     wheelPoint(horizontal_,along1,across[quad],zval_),
		     wheelPoint(horizontal_,along1,across[quad+1],zval_),
		     wheelPoint(horizontal_,along0,across[quad+1],zval_) );
    }

    const float zline = zval_ + cOutlineLift;
    const float alongmin = min[dimension];
    const float alongmax = max[dimension];
    int idx = cNrWheelVertices;
    setQuad( *vertices_, idx,
	     wheelPoint(horizontal_,alongmin,low,zline),
	     wheelPoint(horizontal_,alongmin+cOutlineWidth,low,zline),
	     wheelPoint(horizontal_,alongmin+cOutlineWidth,high,zline),
	     wheelPoint(horizontal_,alongmin,high,zline) );
    idx += cNrQuadVertices;
    setQuad( *vertices_, idx,
	     wheelPoint(horizontal_,alongmax-cOutlineWidth,low,zline),
	     wheelPoint(horizontal_,alongmax,low,zline),
	     wheelPoint(horizontal_,alongmax,high,zline),
	     wheelPoint(horizontal_,alongmax-cOutlineWidth,high,zline) );
    idx += cNrQuadVertices;
    setQuad( *vertices_, idx,
	     wheelPoint(horizontal_,alongmin,low,zline),
	     wheelPoint(horizontal_,alongmax,low,zline),
	     wheelPoint(horizontal_,alongmax,low+cOutlineWidth,zline),
	     wheelPoint(horizontal_,alongmin,low+cOutlineWidth,zline) );
    idx += cNrQuadVertices;
    setQuad( *vertices_, idx,
	     wheelPoint(horizontal_,alongmin,high-cOutlineWidth,zline),
	     wheelPoint(horizontal_,alongmax,high-cOutlineWidth,zline),
	     wheelPoint(horizontal_,alongmax,high,zline),
	     wheelPoint(horizontal_,alongmin,high,zline) );
    vertices_->dirty();
}


void ThumbWheel::updateColors()
{
    const bool glass = style_ == Style::Glass;
    const float halfwidth = glass ? cGlassTickHalfWidth : cTickHalfWidth;
    for ( int slice=0; slice<cNrSlices; slice++ )
    {
	const float anglemid = cPi * (slice+0.5f) / cNrSlices;
	const float sinmid = std::sin( anglemid );
	const float cover = tickCoverage( anglemid+phase_, halfwidth );
	vsg::vec4 endcolor;
	vsg::vec4 bandcolor;
	vsg::vec4 corecolor;
	if ( glass )
	{
	    const float bandalpha = cGlassBandBase + cGlassBandGain*sinmid;
	    const float tickalpha = cover *
				    ( cGlassTickBase + cGlassTickGain*sinmid );
	    const float corealpha = bandalpha + (1.f-bandalpha)*tickalpha;
	    bandcolor = vsg::vec4( 1.f, 1.f, 1.f, bandalpha*opacity_ );
	    endcolor = bandcolor;
	    corecolor = vsg::vec4( 1.f, 1.f, 1.f, corealpha*opacity_ );
	}
	else
	{
	    const float lit = 0.05f + 0.64f*sinmid;
	    const float tick = lit * cover;
	    endcolor = vsg::vec4( lit, lit, lit, opacity_ );
	    bandcolor = corecolor = vsg::vec4( tick, tick, tick, opacity_ );
	}

	const int base = slice * cNrQuadsPerSlice * cNrQuadVertices;
	setQuadColor( *colors_, base, endcolor );
	setQuadColor( *colors_, base+cNrQuadVertices, bandcolor );
	setQuadColor( *colors_, base+2*cNrQuadVertices, corecolor );
	setQuadColor( *colors_, base+3*cNrQuadVertices, bandcolor );
	setQuadColor( *colors_, base+4*cNrQuadVertices, endcolor );
    }

    for ( int idx=0; idx<cNrOutlineQuads; idx++ )
	setQuadColor( *colors_, cNrWheelVertices+idx*cNrQuadVertices,
		      bordercolor_ );

    colors_->dirty();
}


void ThumbWheelHandler::addThumbWheel( ThumbWheel* wheel )
{
    if ( !wheel )
	return;

    vsg::ref_ptr<ThumbWheel> wheelref( wheel );
    if ( std::find(wheels_.begin(),wheels_.end(),wheelref) == wheels_.end() )
	wheels_.push_back( wheelref );
}


void ThumbWheelHandler::removeThumbWheel( ThumbWheel* wheel )
{
    wheels_.erase( std::remove_if(wheels_.begin(),wheels_.end(),
			[wheel]( const vsg::ref_ptr<ThumbWheel>& item )
			{ return item.get()==wheel; }),
		   wheels_.end() );
}


void ThumbWheelHandler::setWindowHeight( uint32_t height )
{
    windowheight_ = height;
}


void ThumbWheelHandler::apply( vsg::ButtonPressEvent& ev )
{
    if ( ev.button == 1 )
	handle( ThumbWheel::Pointer::Press, ev );
}


void ThumbWheelHandler::apply( vsg::ButtonReleaseEvent& ev )
{
    if ( ev.button == 1 )
	handle( ThumbWheel::Pointer::Release, ev );
}


void ThumbWheelHandler::apply( vsg::MoveEvent& ev )
{
    handle( ThumbWheel::Pointer::Move, ev );
}


bool ThumbWheelHandler::handle( ThumbWheel::Pointer type,
				vsg::PointerEvent& ev ) const
{
    vsg::ref_ptr<vsg::Window> window = ev.window.ref_ptr();
    const uint32_t height = window ? window->extent2D().height : windowheight_;
    const float xpos = float( ev.x );
    const float ypos = float( height ) - float( ev.y );
    bool handled = false;
    for ( const vsg::ref_ptr<ThumbWheel>& wheel : wheels_ )
    {
	if ( wheel->handlePointer(type,xpos,ypos) )
	    handled = true;
    }

    if ( handled )
	ev.handled = true;

    return handled;
}

} // namespace vsgGeo
