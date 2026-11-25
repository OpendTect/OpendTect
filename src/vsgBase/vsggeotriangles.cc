/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeotriangles.h"

#include <vsg/core/Visitor.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/VertexDraw.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/RasterizationState.h>
#include <vsg/state/material.h>
#include <vsg/utils/GraphicsPipelineConfigurator.h>
#include <vsg/utils/ShaderSet.h>

namespace
{

struct SetPipelineState : public vsg::Visitor
{
		SetPipelineState( bool blending )
		    : blending_(blending)
		{}

    void apply( vsg::Object& obj ) override
    {
	obj.traverse( *this );
    }

    void apply( vsg::RasterizationState& rs ) override
    {
	rs.cullMode = VK_CULL_MODE_NONE;
    }

    void apply( vsg::ColorBlendState& cbs ) override
    {
	if ( blending_ )
	    cbs.configureAttachments( true );
    }

    bool	blending_;
};

} // namespace


namespace vsgGeo
{

vsg::ref_ptr<vsg::Node> createTriangleList(
			const vsg::ref_ptr<vsg::vec3Array>& vertices,
			const vsg::ref_ptr<vsg::vec4Array>& colors,
			const vsg::ref_ptr<const vsg::Options>& options,
			bool blending )
{
    if ( !vertices || vertices->size() < 3 || !colors ||
	 colors->size() != vertices->size() )
	return {};

    vsg::ref_ptr<vsg::vec3Array> normal = vsg::vec3Array::create( 1 );
    normal->at( 0 ).set( 0.f, 0.f, 1.f );

    vsg::ref_ptr<vsg::ShaderSet> shaderset =
			vsg::createFlatShadedShaderSet( options );
    vsg::ref_ptr<vsg::GraphicsPipelineConfigurator> config =
			vsg::GraphicsPipelineConfigurator::create( shaderset );

    vsg::DataList arrays;
    config->assignArray( arrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX,
			 vertices );
    config->assignArray( arrays, "vsg_Normal", VK_VERTEX_INPUT_RATE_INSTANCE,
			 normal );
    config->assignArray( arrays, "vsg_Color", VK_VERTEX_INPUT_RATE_VERTEX,
			 colors );

    vsg::ref_ptr<vsg::PhongMaterialValue> mat =
			vsg::PhongMaterialValue::create();
    mat->value().diffuse.set( 1.f, 1.f, 1.f, 1.f );
    config->assignDescriptor( "material", mat );

    SetPipelineState pipelinestate( blending );
    config->accept( pipelinestate );
    if ( config->descriptorConfigurator )
	config->descriptorConfigurator->blending = blending;

    config->init();

    vsg::ref_ptr<vsg::VertexDraw> draw = vsg::VertexDraw::create();
    draw->assignArrays( arrays );
    draw->vertexCount = uint32_t( vertices->size() - vertices->size()%3 );
    draw->instanceCount = 1;

    vsg::ref_ptr<vsg::StateGroup> stategroup = vsg::StateGroup::create();
    config->copyTo( stategroup );
    stategroup->addChild( draw );
    return stategroup;
}

} // namespace vsgGeo
