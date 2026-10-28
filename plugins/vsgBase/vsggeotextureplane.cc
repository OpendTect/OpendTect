/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeotextureplane.h"

#include "vsggeolayeredtexturefrag.spv.h"
#include "vsggeolayeredtexturevert.spv.h"

#include <vsg/nodes/DepthSorted.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/VertexIndexDraw.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/RasterizationState.h>
#include <vsg/utils/GraphicsPipelineConfigurator.h>
#include <vsg/utils/ShaderSet.h>

namespace
{

constexpr int32_t cTransparentBin = 10;

/* The sources are in the shaders folder. The SPIR-V is made from them with
   glslangValidator -V --vn <name> -o <name>.spv.h, so that the shaders do not
   have to be compiled when the plugin runs. The stages are registered as the
   variant of the default settings, or VSG would make that variant itself by
   compiling the (here absent) source. */

template <size_t N>
vsg::ShaderModule::SPIRV toSpirv( const uint32_t (&code)[N] )
{
    return vsg::ShaderModule::SPIRV( code, code+N );
}


vsg::ref_ptr<vsg::ShaderSet> createShaderSet( bool blending )
{
    vsg::ref_ptr<vsg::ShaderStage> vert = vsg::ShaderStage::create(
	VK_SHADER_STAGE_VERTEX_BIT, "main", toSpirv(vsgGeoLayeredTextureVert) );
    vsg::ref_ptr<vsg::ShaderStage> frag = vsg::ShaderStage::create(
	VK_SHADER_STAGE_FRAGMENT_BIT, "main", toSpirv(vsgGeoLayeredTextureFrag) );

    vsg::ref_ptr<vsg::ShaderCompileSettings> hints =
				vsg::ShaderCompileSettings::create();
    vsg::ref_ptr<vsg::ShaderSet> shaderset = vsg::ShaderSet::create(
				vsg::ShaderStages{vert,frag}, hints );
    shaderset->variants[hints] = vsg::ShaderStages{ vert, frag };
    shaderset->addAttributeBinding( "vsg_Vertex", "", 0,
				    VK_FORMAT_R32G32B32_SFLOAT,
				    vsg::vec3Array::create(1) );
    shaderset->addAttributeBinding( "vsg_TexCoord0", "", 1,
				    VK_FORMAT_R32G32_SFLOAT,
				    vsg::vec2Array::create(1) );
    shaderset->addDescriptorBinding( "indexMap", "", 0, 0,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				vsg::ubyteArray2D::create(1,1) );
    shaderset->addDescriptorBinding( "colorMap", "", 0, 1,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				vsg::ubvec4Array2D::create(1,1) );
    shaderset->addPushConstantRange( "pc", "", VK_SHADER_STAGE_VERTEX_BIT,
				     0, 128 );

    vsg::ref_ptr<vsg::RasterizationState> rasterstate =
					vsg::RasterizationState::create();
    rasterstate->cullMode = VK_CULL_MODE_NONE;
    shaderset->defaultGraphicsPipelineStates.push_back( rasterstate );

    vsg::ref_ptr<vsg::ColorBlendState> blendstate =
					vsg::ColorBlendState::create();
    blendstate->configureAttachments( blending );
    shaderset->defaultGraphicsPipelineStates.push_back( blendstate );

    return shaderset;
}

} // namespace


namespace vsgGeo
{

TexturePlane::TexturePlane( vsg::ref_ptr<LayeredTexture> texture,
			    const vsg::vec3& origin, const vsg::vec3& uaxis,
			    const vsg::vec3& vaxis, const vsg::vec2& tcorigin,
			    const vsg::vec2& tcu, const vsg::vec2& tcv,
			    vsg::ref_ptr<vsg::SharedObjects> shared,
			    bool blending )
    : texture_(texture)
{
    vsg::ref_ptr<vsg::vec3Array> vertices = vsg::vec3Array::create( 4 );
    vertices->set( 0, origin );
    vertices->set( 1, origin + uaxis );
    vertices->set( 2, origin + uaxis + vaxis );
    vertices->set( 3, origin + vaxis );

    vsg::ref_ptr<vsg::vec2Array> texcoords = vsg::vec2Array::create( 4 );
    texcoords->set( 0, tcorigin );
    texcoords->set( 1, tcorigin + tcu );
    texcoords->set( 2, tcorigin + tcu + tcv );
    texcoords->set( 3, tcorigin + tcv );

    vsg::ref_ptr<vsg::ushortArray> indices = vsg::ushortArray::create( 6 );
    const unsigned short idxs[] = { 0, 1, 2, 0, 2, 3 };
    for ( int idx=0; idx<6; idx++ )
	indices->set( idx, idxs[idx] );

    vsg::ref_ptr<vsg::GraphicsPipelineConfigurator> config =
		vsg::GraphicsPipelineConfigurator::create(
						createShaderSet(blending) );
    vsg::DataList arrays;
    config->assignArray( arrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX,
			 vertices );
    config->assignArray( arrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX,
			 texcoords );
    config->assignTexture( "indexMap",
			   texture_->indexDescriptor(0)->imageInfoList );
    config->assignTexture( "colorMap",
			   texture_->colorDescriptor(1)->imageInfoList );
    config->init();

    vsg::ref_ptr<vsg::VertexIndexDraw> draw = vsg::VertexIndexDraw::create();
    draw->assignArrays( arrays );
    draw->assignIndices( indices );
    draw->indexCount = 6;
    draw->instanceCount = 1;

    vsg::ref_ptr<vsg::StateGroup> stategroup = vsg::StateGroup::create();
    config->copyTo( stategroup, shared );
    stategroup->addChild( draw );
    if ( !blending )
    {
	addChild( stategroup );
	return;
    }

    const vsg::vec3 diagonal = uaxis + vaxis;
    const vsg::dsphere bound( vsg::dvec3(origin+diagonal*0.5f),
			      0.5*double(vsg::length(diagonal)) );
    addChild( vsg::DepthSorted::create( cTransparentBin, bound, stategroup ) );
}


TexturePlane::~TexturePlane()
{}

} // namespace vsgGeo
