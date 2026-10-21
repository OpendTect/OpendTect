/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 vsgGeo runtime updates, runtime compile and delayed release.
 Needs a Vulkan device.

-*/

#include "testprog.h"

#include "vsggeoarrays.h"
#include "vsggeoupdater.h"
#include "vsgtesthelper.h"

#include <random>

namespace
{

class CountedNode : public vsg::Inherit<vsg::Group,CountedNode>
{
public:
			CountedNode()	{ nrlive_++; }

    static int		nrLive()	{ return nrlive_; }

protected:
			~CountedNode()	{ nrlive_--; }

    static std::atomic<int> nrlive_;
};

std::atomic<int> CountedNode::nrlive_{ 0 };


vsg::ref_ptr<vsg::Node> createQuad( vsg::Builder& builder,
				    const vsg::vec4& color, float y=0.f )
{
    vsg::GeometryInfo gi;
    gi.position.set( 0.f, y, 0.f );
    gi.dx.set( 2.f, 0.f, 0.f );
    gi.dy.set( 0.f, 0.f, 2.f );
    gi.color = color;
    vsg::StateInfo si;
    si.lighting = false;
    si.two_sided = true;
    return builder.createQuad( gi, si );
}


vsg::ref_ptr<CountedNode> createTestNode( vsg::Builder& builder,
					  std::mt19937& rng )
{
    std::uniform_real_distribution<float> pos( -1.f, 1.f );
    vsg::GeometryInfo gi;
    gi.dx.set( 0.1f, 0.f, 0.f );
    gi.dy.set( 0.f, 0.1f, 0.f );
    gi.dz.set( 0.f, 0.f, 0.1f );
    gi.color.set( pos(rng)*0.5f+0.5f, pos(rng)*0.5f+0.5f, 1.f, 1.f );

    vsg::ref_ptr<vsg::MatrixTransform> mt = vsg::MatrixTransform::create(
	    vsg::translate(double(pos(rng)),0.0,double(pos(rng))) );
    mt->addChild( builder.createBox(gi,vsg::StateInfo()) );

    vsg::ref_ptr<CountedNode> node = CountedNode::create();
    node->addChild( mt );
    return node;
}

} // namespace


static bool testAddChangeReplaceRemove()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(),
		"Adding and removing nodes opens an off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsgGeo::Compiler& compiler = updater->compiler();
    mRunStandardTest( ov.compile(),
		      "Adding and removing nodes compiles the viewer" );

    vsg::ref_ptr<vsg::Builder> builder = vsg::Builder::create();
    std::mt19937 rng( 42 );
    auto& children = ov.scene().children;
    const size_t maxnrchildren = 200;
    int nradded = 0;

    for ( int frame=0; frame<1000; frame++ )
    {
	const int op = children.empty() ? 0 : int(rng() % 4);
	const size_t idx = children.empty() ? 0 : rng() % children.size();
	if ( op==0 && children.size()<maxnrchildren )
	{
	    vsg::ref_ptr<CountedNode> node = createTestNode( *builder, rng );
	    children.push_back( node );
	    compiler.add( node );
	    nradded++;
	}
	else if ( op==1 )
	{
	    compiler.release( children[idx] );
	    children.erase( children.begin() + idx );
	}
	else if ( op==2 )
	{
	    vsg::ref_ptr<CountedNode> node = createTestNode( *builder, rng );
	    compiler.add( node );
	    compiler.release( children[idx] );
	    children[idx] = node;
	    nradded++;
	}
	else if ( op==3 )
	{
	    auto* grp = children[idx]->cast<vsg::Group>();
	    auto* mt = grp && !grp->children.empty()
		     ? grp->children.front()->cast<vsg::MatrixTransform>()
		     : nullptr;
	    if ( mt )
		mt->matrix = vsg::translate( 0.01, 0.0, 0.0 ) * mt->matrix;
	}

	if ( !ov.frame(1,false) )
	    break;
    }

    for ( auto& child : children )
	compiler.release( child );
    children.clear();
    ov.frame( int(compiler.releaseDelay()) + 2 );

    BufferString msg( "Added ", nradded, " nodes; still alive: " );
    msg.add( CountedNode::nrLive() );
    mRunStandardTestWithError( CountedNode::nrLive()==0,
			       "All removed nodes freed", msg );
    mRunStandardTest( compiler.nrFailedCompiles()==0,
		      "No failed compiles" );
    msg.set( "Messages: " ).add( ov.nrValidationMessages() );
    if ( !ov.hasValidation() )
	msg.add( " (validation layer not available)" );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
		"Adding and removing nodes produces no validation messages",
			       msg );
    return true;
}


static bool testReleaseDelay()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(),
		"Delayed release opens an off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsgGeo::Compiler& compiler = updater->compiler();
    mRunStandardTest( ov.compile(),
		      "Delayed release compiles the viewer" );

    vsg::ref_ptr<vsg::Builder> builder = vsg::Builder::create();
    std::mt19937 rng( 7 );
    {
	vsg::ref_ptr<CountedNode> node = createTestNode( *builder, rng );
	ov.scene().addChild( node );
	compiler.add( node );
    }

    ov.frame( 3, false );
    mRunStandardTest( CountedNode::nrLive()==1, "Node alive in scene" );

    compiler.release( ov.scene().children.back() );
    ov.scene().children.clear();

    const int delay = int( compiler.releaseDelay() );
    ov.frame( delay-1, false );
    mRunStandardTest( CountedNode::nrLive()==1,
		      "Not freed before the frames in flight are done" );

    ov.frame( 1 );
    mRunStandardTest( CountedNode::nrLive()==0,
		      "Freed after the release delay" );
    return true;
}


static bool testDynamicArray()
{
    vsgGeo::DynamicArray<vsg::vec3Array> arr;
    mRunStandardTest( arr.resize(10) && arr.capacity()==16,
		      "First allocation with headroom" );
    mRunStandardTest( arr.array()->properties.dataVariance
			    == vsg::DYNAMIC_DATA,
		      "Array is dynamic" );

    for ( size_t idx=0; idx<16; idx++ )
    {
	arr.resize( idx+1 );
	arr[idx].set( float(idx), 1.f, 2.f );
    }

    const vsg::vec3Array* first = arr.array();
    mRunStandardTest( arr.array()==first && arr.size()==16,
		      "No new allocation below capacity" );

    mRunStandardTest( arr.resize(17) && arr.capacity()==32,
		      "New allocation past capacity" );
    bool sameval = true;
    for ( size_t idx=0; idx<16; idx++ )
	sameval = sameval && arr[idx].x==float(idx) && arr[idx].z==2.f;
    mRunStandardTest( sameval, "Values kept after reallocation" );
    return true;
}


static bool testCompileBeforeAndDuringRun()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(),
		"Compile during the run opens an off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsgGeo::Compiler& compiler = updater->compiler();
    vsg::ref_ptr<vsg::Builder> builder = vsg::Builder::create();

    vsg::ref_ptr<vsg::Node> red =
		createQuad( *builder, vsg::vec4(1.f,0.f,0.f,1.f) );
    ov.scene().addChild( red );
    compiler.add( red );
    mRunStandardTest( ov.compile(),
		      "Compile during the run compiles the viewer" );
    ov.frame( 2 );

    TypeSet<vsg::ubvec4> pixels;
    const uint32_t cx = ov.width()/2, cy = ov.height()/2;
    mRunStandardTest( ov.readColor(pixels),
		      "Read back the quad added before compile" );
    mRunStandardTest( vsgTest::isColor(ov.pixel(pixels,cx,cy),255,0,0),
		      "Node added before compile is drawn" );

    vsg::ref_ptr<vsg::Node> green =
		createQuad( *builder, vsg::vec4(0.f,1.f,0.f,1.f), -1.f );
    ov.scene().addChild( green );
    compiler.add( green );
    ov.frame( 1 );

    mRunStandardTest( ov.readColor(pixels),
		      "Read back the quad added while running" );
    mRunStandardTest( vsgTest::isColor(ov.pixel(pixels,cx,cy),0,255,0),
		      "Node added while running is drawn next frame" );

    BufferString msg( "Messages: ", ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
		"Compile during the run produces no validation messages",
			       msg );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testDynamicArray() ||
	 !testReleaseDelay() ||
	 !testCompileBeforeAndDuringRun() ||
	 !testAddChangeReplaceRemove() )
	return 1;

    return 0;
}
