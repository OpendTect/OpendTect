/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoplanedragger.h"

#include "vsggeolines.h"

#include <vsg/core/Visitor.h>
#include <vsg/nodes/VertexDraw.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/DepthStencilState.h>
#include <vsg/state/InputAssemblyState.h>
#include <vsg/state/material.h>
#include <vsg/utils/GraphicsPipelineConfigurator.h>
#include <vsg/utils/ShaderSet.h>

#include <cmath>

namespace
{

constexpr int32_t cTransparentBin = 10;
constexpr float cOpaqueAlpha = 0.999f;

struct SetBlending : public vsg::Visitor
{
    void apply( vsg::Object& obj ) override
    {
	obj.traverse( *this );
    }

    void apply( vsg::ColorBlendState& cbs ) override
    {
	cbs.configureAttachments( true );
    }
};


vsg::ref_ptr<vsg::Node> createTriangles(
			const vsg::ref_ptr<vsg::vec3Array>& vertices,
			const vsg::ref_ptr<vsg::vec4Array>& colors,
			bool blending=false )
{
    vsg::ref_ptr<vsg::vec3Array> normal = vsg::vec3Array::create( 1 );
    normal->at( 0 ).set( 0.f, 0.f, 1.f );

    vsg::ref_ptr<vsg::ShaderSet> shaderset = vsg::createFlatShadedShaderSet();
    if ( blending )
    {
	bool hasdepthstate = false;
	for ( auto& state : shaderset->defaultGraphicsPipelineStates )
	{
	    vsg::ref_ptr<vsg::DepthStencilState> depthstate =
					state.cast<vsg::DepthStencilState>();
	    if ( depthstate )
	    {
		depthstate->depthWriteEnable = VK_FALSE;
		hasdepthstate = true;
	    }
	}

	if ( !hasdepthstate )
	{
	    vsg::ref_ptr<vsg::DepthStencilState> depthstate =
					vsg::DepthStencilState::create();
	    depthstate->depthWriteEnable = VK_FALSE;
	    shaderset->defaultGraphicsPipelineStates.push_back( depthstate );
	}
    }

    vsg::ref_ptr<vsg::GraphicsPipelineConfigurator> config =
			vsg::GraphicsPipelineConfigurator::create( shaderset );
    vsg::DataList arrays;
    config->assignArray( arrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX,
			 vertices );
    config->assignArray( arrays, "vsg_Normal", VK_VERTEX_INPUT_RATE_INSTANCE,
			 normal );
    const VkVertexInputRate colorrate = colors->size()==vertices->size()
					? VK_VERTEX_INPUT_RATE_VERTEX
					: VK_VERTEX_INPUT_RATE_INSTANCE;
    config->assignArray( arrays, "vsg_Color", colorrate, colors );
    vsg::ref_ptr<vsg::PhongMaterialValue> mat =
			vsg::PhongMaterialValue::create();
    mat->value().diffuse.set( 1.f, 1.f, 1.f, 1.f );
    config->assignDescriptor( "material", mat );
    if ( blending )
    {
	SetBlending setblending;
	config->accept( setblending );
	if ( config->descriptorConfigurator )
	    config->descriptorConfigurator->blending = true;
    }

    config->init();

    vsg::ref_ptr<vsg::VertexDraw> draw = vsg::VertexDraw::create();
    draw->assignArrays( arrays );
    draw->vertexCount = uint32_t( vertices->size() );
    draw->instanceCount = 1;

    vsg::ref_ptr<vsg::StateGroup> stategroup = vsg::StateGroup::create();
    config->copyTo( stategroup );
    stategroup->addChild( draw );
    return stategroup;
}


bool rayPlane( const vsg::dvec3& origin, const vsg::dvec3& direction,
	       const vsg::dvec3& point, const vsg::dvec3& normal,
	       vsg::dvec3& hit )
{
    const double denom = vsg::dot( normal, direction );
    if ( std::abs(denom) < 1e-12 )
	return false;

    const double t = vsg::dot( normal, point-origin ) / denom;
    hit = origin + direction * t;
    return true;
}

} // namespace


namespace vsgGeo
{

vsg::ref_ptr<TabPlaneDragger> createTabPlaneDragger()
{
    return TabPlaneDragger::create();
}


TabPlaneDragger::TabPlaneDragger()
{
    const uint32_t nrverts = uint32_t( 6 * cNrCells * cNrCells );
    planeverts_ = vsg::vec3Array::create( nrverts );
    planeverts_->properties.dataVariance = vsg::DYNAMIC_DATA;
    blendedverts_ = vsg::vec3Array::create( nrverts );
    blendedverts_->properties.dataVariance = vsg::DYNAMIC_DATA;
    lineverts_ = vsg::vec3Array::create( 24 );
    lineverts_->properties.dataVariance = vsg::DYNAMIC_DATA;
    planecolor_ = vsg::vec4Array::create( nrverts );
    planecolor_->properties.dataVariance = vsg::DYNAMIC_DATA;
    for ( auto& col : *planecolor_ )
	col = vsg::vec4( 0.2f, 0.45f, 0.85f, 1.f );
    cellopaque_.assign( cNrCells*cNrCells, 1 );
    addChild( createTriangles(planeverts_,planecolor_) );
    blendedplane_ = vsg::DepthSorted::create( cTransparentBin, vsg::dsphere(),
			    createTriangles(blendedverts_,planecolor_,true) );
    addChild( blendedplane_ );
    addChild( vsgGeo::createLineList(lineverts_,
		vsgGeo::createLineColor(vsg::vec4(1.f,1.f,1.f,1.f))) );
    rebuild();
}


TabPlaneDragger::~TabPlaneDragger()
{}


void TabPlaneDragger::setCenter( const vsg::dvec3& center )
{
    center_ = center;
    clampCenter();
    rebuild();
}


void TabPlaneDragger::setSize( const vsg::dvec3& size )
{
    size_ = size;
    for ( int axis=0; axis<3; axis++ )
    {
	if ( size_[axis] < sizemin_[axis] )
	    size_[axis] = sizemin_[axis];
	if ( size_[axis] > sizemax_[axis] )
	    size_[axis] = sizemax_[axis];
    }

    rebuild();
}


void TabPlaneDragger::setNormalAxis( int axis )
{
    if ( axis>=0 && axis<3 )
	axis_ = axis;
    rebuild();
}


void TabPlaneDragger::setSpaceLimits( int axis, double minv, double maxv )
{
    if ( axis<0 || axis>2 )
	return;

    spacemin_[axis] = minv;
    spacemax_[axis] = maxv;
    clampCenter();
    rebuild();
}


void TabPlaneDragger::setSizeLimits( int axis, double minv, double maxv )
{
    if ( axis<0 || axis>2 )
	return;

    sizemin_[axis] = minv;
    sizemax_[axis] = maxv;
}


void TabPlaneDragger::removeScaleTabs()
{
    handles_ = false;
    rebuild();
}


void TabPlaneDragger::setShowPlane( bool yn )
{
    showplane_ = yn;
    rebuild();
}


void TabPlaneDragger::setShowBorder( bool yn )
{
    showborder_ = yn;
    rebuild();
}


void TabPlaneDragger::setColor( const vsg::vec4& color )
{
    if ( !planecolor_ )
	return;

    for ( auto& col : *planecolor_ )
	col = color;

    planecolor_->dirty();
    cellopaque_.assign( cNrCells*cNrCells, color.a>=cOpaqueAlpha ? 1 : 0 );
    rebuild();
}


int TabPlaneDragger::nrCells()
{
    return cNrCells;
}


void TabPlaneDragger::setCellColors( const vsg::vec4* colors )
{
    if ( !planecolor_ || !colors )
	return;

    for ( int cell=0; cell<cNrCells*cNrCells; cell++ )
    {
	for ( int vert=0; vert<6; vert++ )
	    planecolor_->at( size_t(cell*6 + vert) ) = colors[cell];

	cellopaque_[cell] = colors[cell].a >= cOpaqueAlpha ? 1 : 0;
    }

    planecolor_->dirty();
    rebuild();
}


vsg::dvec3 TabPlaneDragger::axisNormal() const
{
    vsg::dvec3 normal( 0., 0., 0. );
    normal[axis_] = 1.;
    return normal;
}


void TabPlaneDragger::clampCenter()
{
    for ( int axis=0; axis<3; axis++ )
    {
	if ( center_[axis] < spacemin_[axis] )
	    center_[axis] = spacemin_[axis];
	if ( center_[axis] > spacemax_[axis] )
	    center_[axis] = spacemax_[axis];
    }
}


void TabPlaneDragger::rebuild()
{
    const int a = (axis_+1) % 3;
    const int b = (axis_+2) % 3;
    vsg::dvec3 corner[4];
    for ( int idx=0; idx<4; idx++ )
    {
	corner[idx] = center_;
	corner[idx][a] += (idx&1 ? 0.5 : -0.5) * size_[a];
	corner[idx][b] += (idx&2 ? 0.5 : -0.5) * size_[b];
    }

    const vsg::dvec3 hidden = center_;
    const vsg::dvec3 alongu = corner[1] - corner[0];
    const vsg::dvec3 alongv = corner[2] - corner[0];
    auto gridPoint = [&]( int iu, int iv )
    {
	return corner[0] + alongu * ( double(iu) / cNrCells )
			 + alongv * ( double(iv) / cNrCells );
    };
    const vsg::vec3 hiddenvert( float(hidden.x), float(hidden.y),
				float(hidden.z) );
    int trislot = 0;
    auto putTri = [&]( const vsg::dvec3& p )
    {
	const vsg::vec3 vert = showplane_
		? vsg::vec3( float(p.x), float(p.y), float(p.z) )
		: hiddenvert;
	const bool opaque = cellopaque_[ trislot/6 ];
	planeverts_->at( size_t(trislot) ) = opaque ? vert : hiddenvert;
	blendedverts_->at( size_t(trislot) ) = opaque ? hiddenvert : vert;
	trislot++;
    };
    for ( int iv=0; iv<cNrCells; iv++ )
    {
	for ( int iu=0; iu<cNrCells; iu++ )
	{
	    putTri( gridPoint(iu,iv) );
	    putTri( gridPoint(iu+1,iv) );
	    putTri( gridPoint(iu+1,iv+1) );
	    putTri( gridPoint(iu,iv) );
	    putTri( gridPoint(iu+1,iv+1) );
	    putTri( gridPoint(iu,iv+1) );
	}
    }

    planeverts_->dirty();
    blendedverts_->dirty();
    blendedplane_->bound = vsg::dsphere( center_,
			0.5 * vsg::length(corner[3]-corner[0]) );

    vsg::dvec3 edge[4];
    for ( int idx=0; idx<4; idx++ )
	edge[idx] = (corner[idx] + corner[(idx+1)%4]) * 0.5;

    int slot = 0;
    auto putLine = [&]( const vsg::dvec3& p0, const vsg::dvec3& p1, bool draw )
    {
	const vsg::dvec3 a0 = draw ? p0 : hidden;
	const vsg::dvec3 a1 = draw ? p1 : hidden;
	lineverts_->at( slot++ ) = vsg::vec3( float(a0.x), float(a0.y),
					      float(a0.z) );
	lineverts_->at( slot++ ) = vsg::vec3( float(a1.x), float(a1.y),
					      float(a1.z) );
    };

    for ( int idx=0; idx<4; idx++ )
	putLine( corner[idx], corner[(idx+1)%4], showborder_ );

    const double handle = 0.08 * std::max( size_[a], size_[b] );
    for ( int idx=0; idx<4; idx++ )
    {
	vsg::dvec3 tick = corner[idx];
	tick[a] += (idx&1 ? -handle : handle);
	putLine( corner[idx], tick, handles_ );
    }
    for ( int idx=0; idx<4; idx++ )
    {
	vsg::dvec3 tick = edge[idx];
	tick[b] += handle;
	putLine( edge[idx], tick, handles_ );
    }

    lineverts_->dirty();
}


TabPlaneDragger::Hit TabPlaneDragger::hit( const vsg::dvec3& origin,
					   const vsg::dvec3& direction ) const
{
    Hit ret;
    vsg::dvec3 point;
    if ( !rayPlane(origin,direction,center_,axisNormal(),point) )
	return ret;

    const int a = (axis_+1) % 3;
    const int b = (axis_+2) % 3;
    const double ha = 0.5 * size_[a];
    const double hb = 0.5 * size_[b];
    const double da = point[a] - center_[a];
    const double db = point[b] - center_[b];
    if ( std::abs(da) > ha || std::abs(db) > hb )
	return ret;

    ret.point = point;
    ret.part = Plane;
    if ( !handles_ )
	return ret;

    const double reach = 0.12 * std::max( size_[a], size_[b] );
    vsg::dvec3 corner[4];
    for ( int idx=0; idx<4; idx++ )
    {
	corner[idx] = center_;
	corner[idx][a] += (idx&1 ? ha : -ha);
	corner[idx][b] += (idx&2 ? hb : -hb);
	if ( vsg::length(point-corner[idx]) <= reach )
	{
	    ret.part = Handle;
	    ret.handle = idx;
	    return ret;
	}
    }

    return ret;
}


void TabPlaneDragger::begin( const Hit& hit )
{
    dragging_ = hit.part!=None;
    dragpart_ = hit.part;
    draghandle_ = hit.handle;
    beginpoint_ = hit.point;
    begincenter_ = center_;
    beginsize_ = size_;
}


void TabPlaneDragger::move( const vsg::dvec3& point )
{
    if ( !dragging_ )
	return;

    vsg::dvec3 delta = point - beginpoint_;
    if ( translate1d_ || dragpart_==Plane )
    {
	if ( translate1d_ )
	{
	    const vsg::dvec3 normal = axisNormal();
	    delta = normal * vsg::dot( delta, normal );
	}
	else
	    delta[axis_] = 0.;

	setCenter( begincenter_ + delta );
	return;
    }

    const int a = (axis_+1) % 3;
    const int b = (axis_+2) % 3;
    vsg::dvec3 newsize = beginsize_;
    newsize[a] = std::abs( beginsize_[a] + 2.*delta[a] );
    newsize[b] = std::abs( beginsize_[b] + 2.*delta[b] );
    setSize( newsize );
}


void TabPlaneDragger::end()
{
    dragging_ = false;
    dragpart_ = None;
    draghandle_ = -1;
}

} // namespace vsgGeo
