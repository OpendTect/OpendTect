/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 The scene color bar as visBase::SceneColTab is used. No GPU.
-*/

#include "testprog.h"

#include "visvsgdata.h"
#include "visvsgdataman.h"
#include "visvsgfontcreator.h"
#include "visvsgscenecoltab.h"
#include "coltabmapper.h"
#include "thread.h"
#include "vsggeoscalarbar.h"

#include <cmath>

namespace
{

ColTab::Sequence redToBlue()
{
    ColTab::Sequence seq;
    seq.setColor( 0.f, 255, 0, 0 );
    seq.setColor( 1.f, 0, 0, 255 );
    return seq;
}


ColTab::MapperSetup mapperSetup( float start, float stop, bool flip=false )
{
    ColTab::MapperSetup ms;
    ms.range_ = Interval<float>( start, stop );
    ms.flipseq_ = flip;
    return ms;
}


bool isNear( const vsg::vec4& col, float red, float green, float blue,
	     float alpha )
{
    return std::abs(col.r-red)<0.02f && std::abs(col.g-green)<0.02f &&
	   std::abs(col.b-blue)<0.02f && std::abs(col.a-alpha)<0.02f;
}


BufferString describe( const vsg::vec4& col )
{
    BufferString ret( "(" );
    ret.add( col.r ).add( "," ).add( col.g ).add( "," ).add( col.b )
       .add( "," ).add( col.a ).add( ")" );
    return ret;
}


bool hasLabels( const vsgGeo::ScalarBar& bar,
		const std::vector<std::u32string>& expected )
{
    if ( bar.getNumLabels() != int(expected.size()) )
	return false;

    for ( int idx=0; idx<int(expected.size()); idx++ )
    {
	if ( bar.labelText(idx) != expected[idx] )
	    return false;
    }

    return true;
}

} // namespace


static bool testColors()
{
    RefMan<dGBVis::SceneColTab> coltab = dGBVis::SceneColTab::create();
    vsgGeo::ScalarBar* bar = coltab->vsgBar();
    mRunStandardTest( coltab->getLength()==500 && coltab->getThickness()==20,
		      "Scene color bar: The bar is 500 pixels long and 20 "
		      "thick at first" );

    coltab->setColTabSequence( redToBlue() );
    mRunStandardTestWithError( isNear(bar->color(0),1.f,0.f,0.f,1.f) &&
			       isNear(bar->color(255),0.f,0.f,1.f,1.f),
			       "Scene color bar: The bar has the colors of the "
			       "sequence", describe(bar->color(0)).add(" to ")
				.add(describe(bar->color(255))) );
    mRunStandardTestWithError( isNear(bar->color(128),0.5f,0.f,0.5f,1.f),
			       "Scene color bar: The colors in between are "
			       "mixed", describe(bar->color(128)) );

    coltab->setColTabMapperSetup( mapperSetup(10.f,50.f,true) );
    mRunStandardTestWithError( coltab->isSeqFlipped() &&
			       isNear(bar->color(0),0.f,0.f,1.f,1.f) &&
			       isNear(bar->color(255),1.f,0.f,0.f,1.f),
			       "Scene color bar: A flipped sequence is drawn "
			       "the other way around",
			       describe(bar->color(0)) );

    ColTab::Sequence seq = redToBlue();
    seq.setTransparency( Geom::Point2D<float>(0.f,255.f) );
    seq.setTransparency( Geom::Point2D<float>(1.f,0.f) );
    coltab->setColTabMapperSetup( mapperSetup(10.f,50.f,false) );
    coltab->setColTabSequence( seq );
    mRunStandardTestWithError( bar->color(0).a<0.05f && bar->color(255).a>0.95f,
			       "Scene color bar: The transparency of the "
			       "sequence is the alpha of the colors",
			       describe(bar->color(0)).add(" to ")
				.add(describe(bar->color(255))) );
    return true;
}


static bool testRangeAndLabels()
{
    RefMan<dGBVis::SceneColTab> coltab = dGBVis::SceneColTab::create();
    vsgGeo::ScalarBar* bar = coltab->vsgBar();
    coltab->setColTabSequence( redToBlue() );
    coltab->setColTabMapperSetup( mapperSetup(10.f,50.f) );
    mRunStandardTestWithError( hasLabels(*bar,{U"10",U"20",U"30",U"40",U"50"}),
			       "Scene color bar: The labels run from the start "
			       "to the end of the range",
			       BufferString("Labels: ",bar->getNumLabels()) );
    mRunStandardTest( coltab->getRange().start_==10.f &&
		      coltab->getRange().stop_==50.f,
		      "Scene color bar: The range is kept" );

    ColTab::MapperSetup undefined;
    undefined.range_ = Interval<float>( mUdf(float), mUdf(float) );
    coltab->setColTabMapperSetup( undefined );
    mRunStandardTest( hasLabels(*bar,{U"10",U"20",U"30",U"40",U"50"}) &&
		      coltab->getRange().start_==10.f,
		      "Scene color bar: An undefined range is ignored" );

    ColTab::Sequence segmented = redToBlue();
    segmented.setNrSegments( 8 );
    coltab->setAnnotFont( FontData(10,"Arial",FontData::Normal,false) );
    coltab->setColTabSequence( segmented );
    mRunStandardTestWithError( coltab->getNumLabels()==9,
			       "Scene color bar: A sequence with 8 segments "
			       "has a label at each of the 9 limits",
			       BufferString("Labels: ",
					    coltab->getNumLabels()) );

    coltab->setAnnotFont( FontData(60,"Arial",FontData::Normal,false) );
    coltab->setColTabSequence( redToBlue() );
    coltab->setColTabSequence( segmented );
    mRunStandardTestWithError( coltab->getNumLabels()==5,
			       "Scene color bar: With labels too large for "
			       "the bar there are 5",
			       BufferString("Labels: ",
					    coltab->getNumLabels()) );
    return true;
}


static bool testSizeAndPlace()
{
    RefMan<dGBVis::SceneColTab> coltab = dGBVis::SceneColTab::create();
    vsgGeo::ScalarBar* bar = coltab->vsgBar();
    coltab->setWindowSize( 800, 600 );
    mRunStandardTestWithError( bar->getPosition().x==150.f &&
			       bar->getPosition().y==10.f,
			       "Scene color bar: The bar is centered at the "
			       "bottom of the window",
			       BufferString("Position: ",bar->getPosition().x)
				.add(", ").add(bar->getPosition().y) );

    coltab->setSize( 400, 30 );
    coltab->setWindowSize( 1000, 700 );
    mRunStandardTestWithError( coltab->getLength()==400 &&
			       coltab->getThickness()==30 &&
			       bar->getPosition().x==300.f,
			       "Scene color bar: A new size and window size "
			       "move the bar",
			       BufferString("Position: ",
					    bar->getPosition().x) );

    coltab->setSize( 10, 5 );
    mRunStandardTestWithError( coltab->getLength()==350 &&
			       coltab->getThickness()==20,
			       "Scene color bar: The size has a lower limit",
			       BufferString("Size: ",coltab->getLength())
				.add(" x ").add(coltab->getThickness()) );
    coltab->setSize( 5000, 100 );
    mRunStandardTestWithError( coltab->getLength()==1000 &&
			       coltab->getThickness()==35,
			       "Scene color bar: The size has an upper limit",
			       BufferString("Size: ",coltab->getLength())
				.add(" x ").add(coltab->getThickness()) );
    return true;
}


static bool testPositions()
{
    RefMan<dGBVis::SceneColTab> coltab = dGBVis::SceneColTab::create();
    vsgGeo::ScalarBar* bar = coltab->vsgBar();
    coltab->setWindowSize( 800, 600 );
    mRunStandardTest( coltab->getPos()==dGBVis::SceneColTab::Bottom &&
		      coltab->getOrientation(),
		      "Scene color bar: The bar is horizontal at the bottom at "
		      "first" );

    coltab->setPos( dGBVis::SceneColTab::Top );
    mRunStandardTestWithError( coltab->getOrientation() &&
			       bar->getPosition().x==150.f &&
			       bar->getPosition().y==545.f,
			       "Scene color bar: At the top the bar is "
			       "centered with room for the labels above it",
			       BufferString("Position: ",bar->getPosition().x)
				.add(", ").add(bar->getPosition().y) );

    coltab->setPos( dGBVis::SceneColTab::Left );
    const Geom::Size2D<int> leftsz = coltab->getSize();
    const bool vertical = bar->orientation() ==
				vsgGeo::ScalarBar::Orientation::Vertical;
    mRunStandardTestWithError( !coltab->getOrientation() && vertical &&
			       leftsz.width()==20 && leftsz.height()==500 &&
			       bar->getPosition().x==10.f &&
			       bar->getPosition().y==50.f,
			       "Scene color bar: At the left the bar is "
			       "vertical and centered",
			       BufferString("Position: ",bar->getPosition().x)
				.add(", ").add(bar->getPosition().y) );

    coltab->setPos( dGBVis::SceneColTab::Right );
    const float rightx = 800.f - ( 1.5f*float(coltab->getThickness()) +
				   float(coltab->getMinLabelWidth()) );
    mRunStandardTestWithError( bar->getPosition().x==rightx &&
			       bar->getPosition().y==50.f,
			       "Scene color bar: At the right the bar leaves "
			       "room for the labels",
			       BufferString("Position: ",bar->getPosition().x)
				.add(", ").add(bar->getPosition().y) );

    coltab->setSize( 100, 900 );
    mRunStandardTestWithError( coltab->getThickness()==80 &&
			       coltab->getLength()==800,
			       "Scene color bar: The upper limits of a "
			       "vertical bar",
			       BufferString("Size: ",coltab->getThickness())
				.add(" x ").add(coltab->getLength()) );
    coltab->setSize( 5, 10 );
    mRunStandardTestWithError( coltab->getThickness()==20 &&
			       coltab->getLength()==350,
			       "Scene color bar: The lower limits of a "
			       "vertical bar",
			       BufferString("Size: ",coltab->getThickness())
				.add(" x ").add(coltab->getLength()) );

    coltab->setPos( dGBVis::SceneColTab::Bottom );
    coltab->setSize( 1000, 35 );
    coltab->setPos( dGBVis::SceneColTab::Left );
    mRunStandardTestWithError( coltab->getLength()==800 &&
			       coltab->getThickness()==35,
			       "Scene color bar: A new orientation brings the "
			       "size within the limits",
			       BufferString("Size: ",coltab->getThickness())
				.add(" x ").add(coltab->getLength()) );
    return true;
}


static bool testVerticalLabels()
{
    RefMan<dGBVis::SceneColTab> coltab = dGBVis::SceneColTab::create();
    coltab->setWindowSize( 800, 600 );
    coltab->setPos( dGBVis::SceneColTab::Left );
    ColTab::Sequence segmented = redToBlue();
    segmented.setNrSegments( 8 );
    coltab->setAnnotFont( FontData(60,"Arial",FontData::Normal,false) );
    coltab->setColTabSequence( segmented );
    mRunStandardTestWithError( coltab->getNumLabels()==9,
			       "Scene color bar: In a vertical bar large "
			       "labels only have to fit on top of each other",
			       BufferString("Labels: ",
					    coltab->getNumLabels()) );

    coltab->setSize( 20, 350 );
    mRunStandardTestWithError( coltab->getNumLabels()==5,
			       "Scene color bar: With labels too large for a "
			       "short vertical bar there are 5",
			       BufferString("Labels: ",
					    coltab->getNumLabels()) );
    return true;
}


static bool testSwitchedOff()
{
    RefMan<dGBVis::SceneColTab> coltab = dGBVis::SceneColTab::create();
    vsgGeo::ScalarBar* bar = coltab->vsgBar();
    coltab->setColTabSequence( redToBlue() );
    coltab->turnOn( false );
    ColTab::Sequence other;
    other.setColor( 0.f, 0, 255, 0 );
    other.setColor( 1.f, 255, 255, 0 );
    coltab->setColTabSequence( other );
    mRunStandardTest( isNear(bar->color(0),1.f,0.f,0.f,1.f),
		      "Scene color bar: A bar that is switched off keeps its "
		      "colors" );

    coltab->turnOn( true );
    mRunStandardTestWithError( isNear(bar->color(0),0.f,1.f,0.f,1.f) &&
			       isNear(bar->color(255),1.f,1.f,0.f,1.f),
			       "Scene color bar: Switching it on shows the "
			       "sequence that was set meanwhile",
			       describe(bar->color(0)) );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    dGBVis::DataObject::setVisualizationThread( Threads::currentThread() );
    dGBVis::initStdClasses();

    const bool res = testColors() && testRangeAndLabels() &&
		     testSizeAndPlace() && testPositions() &&
		     testVerticalLabels() && testSwitchedOff();
    dGBVis::FontCreator::instance().clear();
    if ( !res )
	return 1;

    return 0;
}
