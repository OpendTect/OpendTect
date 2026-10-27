/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 Pick masks and which hit is nearest. Needs a Vulkan device for
 the camera.
-*/

#include "testprog.h"

#include "visvsgdataman.h"
#include "visvsgobject.h"
#include "thread.h"
#include "vsggeolines.h"
#include "vsggeopick.h"
#include "vsgtesthelper.h"

#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/VertexDraw.h>
#include <vsg/state/material.h>
#include <vsg/utils/GraphicsPipelineConfigurator.h>
#include <vsg/utils/ShaderSet.h>

namespace
{

class PickItem : public dGBVis::VisualObjectImpl
{
public:
    static RefMan<PickItem> create()
    {
	RefMan<PickItem> ret = new PickItem;
	return ret;
    }

    void addNode( vsg::Node* node )	{ addChild( node ); }

private:
		PickItem() : VisualObjectImpl(false)	{}
		~PickItem()				{}
};


vsg::ref_ptr<vsg::Node> triangleAt( float y )
{
    vsg::ref_ptr<vsg::vec3Array> vertices = vsg::vec3Array::create( 6 );
    const float x0 = -0.8f, x1 = 0.8f, z0 = -0.8f, z1 = 0.8f;
    const vsg::vec3 p[4] = {
	vsg::vec3( x0, y, z0 ), vsg::vec3( x1, y, z0 ),
	vsg::vec3( x1, y, z1 ), vsg::vec3( x0, y, z1 ) };
    const int order[6] = { 0, 1, 2, 0, 2, 3 };
    for ( int idx=0; idx<6; idx++ )
	vertices->at( idx ) = p[order[idx]];

    vsg::ref_ptr<vsg::ShaderSet> shaderset = vsg::createFlatShadedShaderSet();
    vsg::ref_ptr<vsg::GraphicsPipelineConfigurator> config =
			vsg::GraphicsPipelineConfigurator::create( shaderset );
    vsg::DataList arrays;
    config->assignArray( arrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX,
			 vertices );
    vsg::ref_ptr<vsg::vec4Array> colors = vsg::vec4Array::create( 1 );
    colors->at( 0 ) = vsg::vec4( 1.f, 0.f, 0.f, 1.f );
    config->assignArray( arrays, "vsg_Color", VK_VERTEX_INPUT_RATE_INSTANCE,
			 colors );
    vsg::ref_ptr<vsg::PhongMaterialValue> mat =
			vsg::PhongMaterialValue::create();
    mat->value().diffuse.set( 1.f, 1.f, 1.f, 1.f );
    config->assignDescriptor( "material", mat );
    config->init();

    vsg::ref_ptr<vsg::VertexDraw> draw = vsg::VertexDraw::create();
    draw->assignArrays( arrays );
    draw->vertexCount = 6;
    draw->instanceCount = 1;
    vsg::ref_ptr<vsg::StateGroup> stategroup = vsg::StateGroup::create();
    config->copyTo( stategroup );
    stategroup->addChild( draw );
    return stategroup;
}


VisID hitId( const vsg::Camera& camera, vsg::Node& root, int x, int y,
	     vsg::Mask mask )
{
    const vsgGeo::Pick hit = vsgGeo::intersect( camera, root, x, y, mask, 1. );
    if ( !hit.valid )
	return VisID::udf();

    for ( auto it=hit.nodePath.rbegin(); it!=hit.nodePath.rend(); ++it )
    {
	const VisID id = dGBVis::DataObject::getID( *it );
	if ( id.isValid() )
	    return id;
    }

    return VisID::udf();
}

} // namespace


static bool testPicks( vsgTest::OffscreenViewer& viewer )
{
    const int cx = int( viewer.width() / 2 );
    const int cy = int( viewer.height() / 2 );

    RefMan<PickItem> active = PickItem::create();
    RefMan<PickItem> passive = PickItem::create();
    active->setPickable( true, false );
    passive->setPickable( false, true );
    active->addNode( triangleAt(0.f).get() );
    passive->addNode( triangleAt(0.f).get() );
    viewer.scene().addChild( vsg::ref_ptr<vsg::Node>(active->vsgNode()) );
    viewer.scene().addChild( vsg::ref_ptr<vsg::Node>(passive->vsgNode()) );

    const VisID activehit = hitId( viewer.camera(), viewer.scene(), cx, cy,
				   dGBVis::cActivePickMask );
    const VisID passivehit = hitId( viewer.camera(), viewer.scene(), cx, cy,
				    dGBVis::cPassivePickMask );
    mRunStandardTest( activehit==active->id() && passivehit==passive->id(),
		      "Active and passive masks pick different objects" );

    active->turnOn( false );
    const VisID hidden = hitId( viewer.camera(), viewer.scene(), cx, cy,
				dGBVis::cActivePickMask );
    mRunStandardTest( !hidden.isValid(), "A hidden object is not hit" );
    active->turnOn( true );

    viewer.scene().children.clear();
    RefMan<PickItem> quad = PickItem::create();
    quad->setPickable( true, true );
    quad->addNode( triangleAt(0.f).get() );
    viewer.scene().addChild( vsg::ref_ptr<vsg::Node>(quad->vsgNode()) );
    const vsgGeo::Pick center = vsgGeo::intersect(
		viewer.camera(), viewer.scene(), cx, cy,
		dGBVis::cActivePickMask, 1. );
    mRunStandardTest( center.valid &&
		      std::abs(center.worldPoint.x)<0.05 &&
		      std::abs(center.worldPoint.z)<0.05,
		      "The view center hits the quad center" );

    const vsgGeo::Pick miss = vsgGeo::intersect(
		viewer.camera(), viewer.scene(), 0, 0,
		dGBVis::cActivePickMask, 1. );
    mRunStandardTest( !miss.valid, "A corner pixel misses the quad" );

    const float dy = float( 0.5 * 2.0 / viewer.height() );
    RefMan<PickItem> line = PickItem::create();
    line->setPickable( true, true );
    vsg::ref_ptr<vsg::vec3Array> lineverts = vsg::vec3Array::create( 2 );
    lineverts->at( 0 ) = vsg::vec3( -0.8f, -dy, 0.f );
    lineverts->at( 1 ) = vsg::vec3( 0.8f, -dy, 0.f );
    line->addNode( vsgGeo::createLineList(lineverts,
		vsgGeo::createLineColor(vsg::vec4(0.f,1.f,0.f,1.f))).get() );
    viewer.scene().addChild( vsg::ref_ptr<vsg::Node>(line->vsgNode()) );
    const VisID front = hitId( viewer.camera(), viewer.scene(), cx, cy,
			       dGBVis::cActivePickMask );
    mRunStandardTest( front==line->id(),
		      "A line in front of a triangle wins" );

    viewer.scene().children.clear();
    RefMan<PickItem> nearquad = PickItem::create();
    RefMan<PickItem> farquad = PickItem::create();
    nearquad->setPickable( true, true );
    farquad->setPickable( true, true );
    nearquad->addNode( triangleAt(0.f).get() );
    farquad->addNode( triangleAt(1.f).get() );
    viewer.scene().addChild( vsg::ref_ptr<vsg::Node>(nearquad->vsgNode()) );
    viewer.scene().addChild( vsg::ref_ptr<vsg::Node>(farquad->vsgNode()) );
    const VisID nearer = hitId( viewer.camera(), viewer.scene(), cx, cy,
				dGBVis::cActivePickMask );
    mRunStandardTest( nearer==nearquad->id(),
		      "The nearer quad wins" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    dGBVis::DataObject::setVisualizationThread( Threads::currentThread() );
    dGBVis::initStdClasses();

    vsgTest::OffscreenViewer viewer( 64, 64, false );
    if ( !viewer.isOK() )
    {
	errStream() << viewer.errMsg().c_str() << od_endl;
	return 1;
    }

    if ( !testPicks(viewer) )
	return 1;

    return 0;
}
