/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 The dGBVis object model, and rendering of the annotation box
 and grid lines. Needs a Vulkan device.

-*/

#include "testprog.h"

#include "visvsgannotation.h"
#include "visvsgdatagroup.h"
#include "visvsgdataman.h"
#include "visvsgobject.h"
#include "visvsgtransform.h"
#include "iopar.h"
#include "thread.h"
#include "vsggeoupdater.h"
#include "vsgtesthelper.h"

#include <random>

namespace
{

class TestShape : public dGBVis::VisualObjectImpl
{
public:

    static RefMan<TestShape> createBox( vsg::Builder& builder, float size )
    {
	RefMan<TestShape> ret = new TestShape;
	vsg::GeometryInfo gi;
	gi.dx.set( size, 0.f, 0.f );
	gi.dy.set( 0.f, size, 0.f );
	gi.dz.set( 0.f, 0.f, size );
	ret->addChild( builder.createBox(gi,vsg::StateInfo()) );
	return ret;
    }

    static RefMan<TestShape> createQuad( vsg::Builder& builder,
					 const vsg::vec4& color )
    {
	RefMan<TestShape> ret = new TestShape;
	vsg::GeometryInfo gi;
	gi.dx.set( 2.f, 0.f, 0.f );
	gi.dy.set( 0.f, 0.f, 2.f );
	gi.color = color;
	vsg::StateInfo si;
	si.lighting = false;
	si.two_sided = true;
	ret->addChild( builder.createQuad(gi,si) );
	return ret;
    }

protected:
			TestShape() : VisualObjectImpl(false)	{}
			~TestShape()				{}
};


class SceneSetup
{
public:
    SceneSetup( vsgTest::OffscreenViewer& ov )
	: ov_(ov)
    {
	dGBVis::DataObject::setCommonViewer( &ov.viewer() );
	root_ = dGBVis::DataObjectGroup::create();
	ov.scene().addChild( vsg::ref_ptr<vsg::Node>(root_->vsgNode()) );
    }

    ~SceneSetup()
    {
	root_ = nullptr;
	ov_.scene().children.clear();
	ov_.viewer().deviceWaitIdle();
	dGBVis::DataObject::setCommonViewer( nullptr );
    }

    vsgTest::OffscreenViewer&		ov_;
    RefMan<dGBVis::DataObjectGroup>	root_;
};

} // namespace


static bool testRandomTree()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(),
		"Random object tree opens an off-screen viewer",
			       ov.errMsg().c_str() );

    const int nrobjsatstart = dGBVis::DM().nrObjects();
    {
	SceneSetup setup( ov );
	mRunStandardTest( ov.compile(),
			  "Random object tree compiles the viewer" );

	vsg::ref_ptr<vsg::Builder> builder = vsg::Builder::create();
	std::mt19937 rng( 1234 );
	RefObjectSet<dGBVis::DataObjectGroup> groups;
	groups += setup.root_.ptr();
	RefObjectSet<dGBVis::DataObject> objects;

	for ( int step=0; step<1000; step++ )
	{
	    dGBVis::DataObjectGroup* grp = groups[ rng() % groups.size() ];
	    const int op = int( rng() % 5 );
	    if ( op==0 || op==1 )
	    {
		RefMan<TestShape> shape
				    = TestShape::createBox( *builder, 0.1f );
		grp->addObject( shape.ptr() );
		objects += shape.ptr();
	    }
	    else if ( op==2 )
	    {
		RefMan<dGBVis::DataObjectGroup> newgrp =
					dGBVis::DataObjectGroup::create();
		grp->addObject( newgrp.ptr() );
		groups += newgrp.ptr();
		objects += newgrp.ptr();
	    }
	    else if ( op==3 && !objects.isEmpty() )
	    {
		dGBVis::DataObject* obj = objects[ rng() % objects.size() ];
		obj->turnOn( !obj->isOn() );
	    }
	    else if ( op==4 && grp->size() )
	    {
		const int idx = int( rng() % grp->size() );
		const dGBVis::DataObject* obj = grp->getObject( idx );
		grp->removeObject( idx );
		const int objidx = objects.indexOf( obj );
		if ( objects.validIdx(objidx) )
		    objects.removeSingle( objidx );
	    }

	    if ( !ov.frame(1,false) )
		break;
	}

	for ( auto* grp : groups )
	    grp->removeAll();
	groups.erase();
	objects.erase();

	const int delay =
	    int( dGBVis::DataObject::getUpdater()->compiler().releaseDelay() );
	ov.frame( delay+2 );
    }

    BufferString msg( "Objects left: ",
		      dGBVis::DM().nrObjects()-nrobjsatstart );
    mRunStandardTestWithError( dGBVis::DM().nrObjects()==nrobjsatstart,
			       "DataManager empty after removing all",
			       msg );
    msg.set( "Messages: " ).add( ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
		"Random object tree produces no validation messages", msg );
    return true;
}


static bool testTurnOn()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(),
		"Turning an object on and off opens an off-screen viewer",
			       ov.errMsg().c_str() );

    SceneSetup setup( ov );
    vsg::ref_ptr<vsg::Builder> builder = vsg::Builder::create();
    RefMan<TestShape> quad =
		TestShape::createQuad( *builder, vsg::vec4(1.f,0.f,0.f,1.f) );
    setup.root_->addObject( quad.ptr() );
    mRunStandardTest( ov.compile(),
		      "Turning an object on and off compiles the viewer" );

    TypeSet<vsg::ubvec4> pixels;
    const uint32_t cx = ov.width()/2, cy = ov.height()/2;
    ov.frame( 2 );
    mRunStandardTest( ov.readColor(pixels) &&
		      vsgTest::isColor(ov.pixel(pixels,cx,cy),255,0,0),
		      "Object drawn when on" );

    quad->turnOn( false );
    ov.frame( 1 );
    mRunStandardTest( ov.readColor(pixels) &&
		      vsgTest::isColor(ov.pixel(pixels,cx,cy),0,0,0),
		      "Background shows when off" );

    quad->turnOn( true );
    ov.frame( 1 );
    mRunStandardTest( ov.readColor(pixels) &&
		      vsgTest::isColor(ov.pixel(pixels,cx,cy),255,0,0),
		      "Object drawn again when on" );

    BufferString msg( "Messages: ", ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
		"Turning an object on and off produces no validation messages",
			       msg );
    return true;
}


static bool testBoundingBox()
{
    vsg::ref_ptr<vsg::Builder> builder = vsg::Builder::create();
    RefMan<TestShape> box = TestShape::createBox( *builder, 0.2f );
    Coord3 minpos, maxpos;
    mRunStandardTest( box->getBoundingBox(minpos,maxpos),
		      "Bounding box available" );

    const Coord3 size = maxpos - minpos;
    BufferString msg( "Size: " );
    msg.add( size.x_ ).add( ", " ).add( size.y_ ).add( ", " ).add( size.z_ );
    mRunStandardTestWithError( mIsEqual(size.x_,0.2,1e-6) &&
			       mIsEqual(size.y_,0.2,1e-6) &&
			       mIsEqual(size.z_,0.2,1e-6),
			       "Bounding box size", msg );
    return true;
}


static bool testParRoundTrip()
{
    vsg::ref_ptr<vsg::Builder> builder = vsg::Builder::create();
    RefMan<TestShape> obj1 = TestShape::createBox( *builder, 0.1f );
    obj1->turnOn( false );
    IOPar par1;
    obj1->fillPar( par1 );

    RefMan<TestShape> obj2 = TestShape::createBox( *builder, 0.1f );
    obj2->usePar( par1 );
    IOPar par2;
    obj2->fillPar( par2 );

    mRunStandardTest( !obj2->isOn(), "usePar restores on/off" );
    mRunStandardTest( par1.isEqual(par2), "fillPar round trip" );
    return true;
}


static bool isLit( const vsg::ubvec4& pix )
{
    return pix.r>40 || pix.g>40 || pix.b>40;
}


static int nrLitPixels( vsgTest::OffscreenViewer& ov,
			const TypeSet<vsg::ubvec4>& pixels,
			uint32_t x0, uint32_t x1, uint32_t y0, uint32_t y1 )
{
    int nr = 0;
    for ( uint32_t y=y0; y<=y1; y++ )
	for ( uint32_t x=x0; x<=x1; x++ )
	    if ( isLit(ov.pixel(pixels,x,y)) )
		nr++;

    return nr;
}


static bool testAnnotationGrid()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(),
		"Annotation grid opens an off-screen viewer",
			       ov.errMsg().c_str() );

    SceneSetup setup( ov );

    // Inline 100-200 on X, crossline 300-400 on Y (away from the camera),
    // Z 0-1 downwards, all to [-0.8,0.8] in display space.
    RefMan<dGBVis::Transformation> trans = dGBVis::Transformation::create();
    trans->setA( 0.016, 0, 0, -150*0.016,
		 0, 0.016, 0, -350*0.016,
		 0, 0, -1.6, 0.8,
		 0, 0, 0, 1 );

    TrcKeyZSampling tkzs( false );
    tkzs.hsamp_.start_ = BinID( 100, 300 );
    tkzs.hsamp_.stop_ = BinID( 200, 400 );
    tkzs.hsamp_.step_ = BinID( 1, 1 );
    tkzs.zsamp_.set( 0.f, 1.f, 0.004f );

    RefMan<dGBVis::Annotation> annot = dGBVis::Annotation::create();
    annot->setDisplayTransformation( trans.ptr() );
    annot->setTrcKeyZSampling( tkzs );
    annot->setColor( OD::Color(0,255,0) );
    setup.root_->addObject( annot.ptr() );
    mRunStandardTest( ov.compile(),
		      "Annotation grid compiles the viewer" );

    int nrgridsegs = 0;
    for ( int side=0; side<6; side++ )
	nrgridsegs += annot->nrGridLineSegments( side );
    mRunStandardTest( nrgridsegs>0 && !annot->getTicks().empty(),
		      "Grid lines and ticks computed" );

    const uint32_t w = ov.width(), h = ov.height();
    const uint32_t x0 = w/5, x1 = w*4/5, y0 = h/5, y1 = h*4/5;
    const uint32_t cx = w/2;
    const uint32_t edgey0 = uint32_t( 0.1f*h ) - 2;
    const uint32_t edgey1 = uint32_t( 0.1f*h ) + 2;

    TypeSet<vsg::ubvec4> pixels;
    ov.frame( 2 );
    mRunStandardTest( ov.readColor(pixels) &&
		      nrLitPixels(ov,pixels,cx,cx,edgey0,edgey1)>0,
		      "Box drawn" );
    mRunStandardTest( nrLitPixels(ov,pixels,x0,x1,y0,y1)==0,
		      "No grid lines when grid is off" );

    annot->showGridLines( true );
    ov.frame( 2 );
    mRunStandardTest( ov.readColor(pixels) &&
		      nrLitPixels(ov,pixels,x0,x1,y0,y1)>0,
		      "Grid lines drawn on the back side" );

    annot->showGridLines( false );
    ov.frame( 1 );
    mRunStandardTest( ov.readColor(pixels) &&
		      nrLitPixels(ov,pixels,x0,x1,y0,y1)==0,
		      "Grid lines gone when switched off" );

    annot->turnOn( false );
    ov.frame( 1 );
    mRunStandardTest( ov.readColor(pixels) &&
		      nrLitPixels(ov,pixels,0,w-1,0,h-1)==0,
		      "Nothing drawn when annotation is off" );

    BufferString msg( "Messages: ", ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
		"Annotation grid produces no validation messages", msg );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    dGBVis::DataObject::setVisualizationThread( Threads::currentThread() );
    dGBVis::initStdClasses();

    if ( !testParRoundTrip() ||
	 !testBoundingBox() ||
	 !testTurnOn() ||
	 !testRandomTree() ||
	 !testAnnotationGrid() )
	return 1;

    return 0;
}
