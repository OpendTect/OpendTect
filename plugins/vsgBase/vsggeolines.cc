/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeolines.h"

#include <vsg/core/Visitor.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/VertexDraw.h>
#include <vsg/state/InputAssemblyState.h>
#include <vsg/state/material.h>
#include <vsg/utils/GraphicsPipelineConfigurator.h>
#include <vsg/utils/ShaderSet.h>

namespace
{

struct SetLineListTopology : public vsg::Visitor
{
    void apply( vsg::Object& obj ) override
    { obj.traverse( *this ); }
    void apply( vsg::InputAssemblyState& ias ) override
    { ias.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; }
};

} // namespace


namespace vsgGeo
{

vsg::ref_ptr<vsg::vec4Array> createLineColor( const vsg::vec4& col )
{
    vsg::ref_ptr<vsg::vec4Array> ret = vsg::vec4Array::create( 1 );
    ret->at( 0 ) = col;
    ret->properties.dataVariance = vsg::DYNAMIC_DATA;
    return ret;
}


vsg::ref_ptr<vsg::Node> createLineList(
			const vsg::ref_ptr<vsg::vec3Array>& vertices,
			const vsg::ref_ptr<vsg::vec4Array>& color,
			const vsg::ref_ptr<const vsg::Options>& options )
{
    if ( !vertices || vertices->size() < 2 || !color || color->size() < 1 )
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
    config->assignArray( arrays, "vsg_Color", VK_VERTEX_INPUT_RATE_INSTANCE,
			 color );

    vsg::ref_ptr<vsg::PhongMaterialValue> mat =
			vsg::PhongMaterialValue::create();
    mat->value().diffuse.set( 1.f, 1.f, 1.f, 1.f );
    config->assignDescriptor( "material", mat );

    SetLineListTopology topology;
    config->accept( topology );
    config->init();

    vsg::ref_ptr<vsg::VertexDraw> draw = vsg::VertexDraw::create();
    draw->assignArrays( arrays );
    draw->vertexCount = uint32_t( vertices->size() - vertices->size()%2 );
    draw->instanceCount = 1;

    vsg::ref_ptr<vsg::StateGroup> stategroup = vsg::StateGroup::create();
    config->copyTo( stategroup );
    stategroup->addChild( draw );
    return stategroup;
}

} // namespace vsgGeo
