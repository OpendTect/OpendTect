/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "vsggeoscalarbar.h"
#include "vsgtestassert.h"
#include "vsgtesthelper.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

namespace
{

constexpr uint32_t cSize = 256;
constexpr int cBarLeft = 28;
constexpr int cBarLength = 200;
constexpr int cBarBottom = 20;
constexpr int cBarThickness = 20;
constexpr int cBarMiddle = cBarBottom + cBarThickness/2;
constexpr int cBarTop = cBarBottom + cBarThickness;
constexpr int cVertLeft = 40;
constexpr int cVertBottom = 24;
constexpr int cVertRight = cVertLeft + cBarThickness;
constexpr int cVertMiddle = cVertLeft + cBarThickness/2;
constexpr int cVertTop = cVertBottom + cBarLength;

using Pixels = TypeSet<vsg::ubvec4>;


vsg::ref_ptr<vsg::Font> loadFont()
{
    const std::filesystem::path dir =
	    std::filesystem::path( __FILE__ ).parent_path().parent_path();
    return vsg::read_cast<vsg::Font>(
		(dir / "folder_vsg" / "times.vsgb").string() );
}


vsg::ubvec4 pixelAt( const Pixels& pixels, int x, int yhud )
{
    return pixels[ int((cSize-1-yhud)*cSize + x) ];
}


bool isWhite( const vsg::ubvec4& pix )
{
    return pix.r>200 && pix.g>200 && pix.b>200;
}


int countWhite( const Pixels& pixels, int xmin, int xmax, int ymin, int ymax )
{
    int ret = 0;
    for ( int y=ymin; y<=ymax; y++ )
    {
	for ( int x=xmin; x<=xmax; x++ )
	{
	    if ( isWhite(pixelAt(pixels,x,y)) )
		ret++;
	}
    }

    return ret;
}


BufferString describe( const vsg::ubvec4& pix )
{
    BufferString ret( "(" );
    ret.add( int(pix.r) ).add( "," ).add( int(pix.g) ).add( "," )
       .add( int(pix.b) ).add( ")" );
    return ret;
}


std::vector<vsg::vec4> redToBlue( float alpha=1.f )
{
    std::vector<vsg::vec4> colors( vsgGeo::ScalarBar::cNrColors );
    for ( int idx=0; idx<int(colors.size()); idx++ )
    {
	const float level = float(idx) / float(colors.size()-1);
	colors[idx] = vsg::vec4( 1.f-level, 0.f, level, alpha );
    }

    return colors;
}


void setColors( vsgGeo::ScalarBar& bar, const std::vector<vsg::vec4>& colors )
{
    bar.setColors( colors.data(), int(colors.size()) );
}


struct HudBar
{
		HudBar( vsgTest::OffscreenViewer& ov )
		    : bar_(vsgGeo::ScalarBar::create())
		{
		    vsgGeo::Text::setViewportHeight( float(ov.height()) );
		    ov.addHud()->addChild( bar_ );
		    bar_->setSize( float(cBarLength), float(cBarThickness) );
		    bar_->setPosition( vsg::vec2(float(cBarLeft),
						 float(cBarBottom)) );
		    setColors( *bar_, redToBlue() );
		    bar_->setRange( 0.f, 100.f );
		}

    vsg::ref_ptr<vsgGeo::ScalarBar>	bar_;
};


void makeVertical( vsgGeo::ScalarBar& bar )
{
    bar.setOrientation( vsgGeo::ScalarBar::Orientation::Vertical );
    bar.setPosition( vsg::vec2(float(cVertLeft),float(cVertBottom)) );
}

} // namespace


static bool testColors()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Color bar: Off-screen viewer",
			       ov.errMsg().c_str() );

    HudBar hudbar( ov );
    mRunStandardTest( ov.compile(), "Color bar: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Color bar: Image is read" );
    const vsg::ubvec4 left = pixelAt( pixels, cBarLeft+5, cBarMiddle );
    const vsg::ubvec4 middle = pixelAt( pixels, cBarLeft+cBarLength/2,
					cBarMiddle );
    const vsg::ubvec4 right = pixelAt( pixels, cBarLeft+cBarLength-5,
				       cBarMiddle );
    mRunStandardTestWithError( left.r>230 && left.b<30 && left.g<10,
			       "Color bar: The bar starts with the first color",
			       describe(left) );
    mRunStandardTestWithError( std::abs(int(middle.r)-128)<20 &&
			       std::abs(int(middle.b)-127)<20 && middle.g<10,
			       "Color bar: The middle of the bar has the color "
			       "in the middle of the table", describe(middle) );
    mRunStandardTestWithError( right.b>230 && right.r<30 && right.g<10,
			       "Color bar: The bar ends with the last color",
			       describe(right) );
    const vsg::ubvec4 outside = pixelAt( pixels, cBarLeft-10, cBarMiddle );
    const vsg::ubvec4 beyond = pixelAt( pixels, cBarLeft+cBarLength+10,
					cBarMiddle );
    mRunStandardTestWithError( outside.r<10 && outside.b<10 &&
			       beyond.r<10 && beyond.b<10,
			       "Color bar: Nothing is drawn beside the bar",
			       describe(outside) );

    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Color bar: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testBorderAndTicks()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(),
			       "Color bar border: Off-screen viewer",
			       ov.errMsg().c_str() );

    HudBar hudbar( ov );
    mRunStandardTest( ov.compile(), "Color bar border: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels),
		      "Color bar border: Image is read" );
    const int top = countWhite( pixels, cBarLeft+10, cBarLeft+cBarLength-10,
				cBarTop-1, cBarTop+1 );
    const int bottom = countWhite( pixels, cBarLeft+10,
				   cBarLeft+cBarLength-10, cBarBottom-1,
				   cBarBottom+1 );
    mRunStandardTestWithError( top>=cBarLength-30 && bottom>=cBarLength-30,
			       "Color bar border: The bar has a white border "
			       "along its long sides",
			       BufferString("Top: ",top).add(", bottom: ")
							.add(bottom) );

    const int nrticks = hudbar.bar_->getNumLabels();
    mRunStandardTestWithError( nrticks==5,
			       "Color bar border: There are 5 labels at first",
			       BufferString("Labels: ",nrticks) );
    int nrmissing = 0;
    for ( int idx=0; idx<nrticks; idx++ )
    {
	const int x = cBarLeft + idx*cBarLength/(nrticks-1);
	const int found = countWhite( pixels, x-1, x+1, cBarTop+3,
				      cBarTop+8 );
	if ( found < 6 )
	    nrmissing++;
    }

    mRunStandardTestWithError( nrmissing==0,
			       "Color bar border: Every label has a tick above "
			       "the bar", BufferString("Missing: ",nrmissing) );

    hudbar.bar_->setNumLabels( 3 );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Color bar border: Second image is read" );
    const int removed = countWhite( pixels, cBarLeft+cBarLength/4-1,
				    cBarLeft+cBarLength/4+1, cBarTop+3,
				    cBarTop+8 );
    mRunStandardTestWithError( removed==0,
			       "Color bar border: Fewer labels leave out the "
			       "ticks in between",
			       BufferString("Pixels: ",removed) );
    return true;
}


static bool testLabels()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Color bar labels: Font loaded" );

    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(),
			       "Color bar labels: Off-screen viewer",
			       ov.errMsg().c_str() );

    HudBar hudbar( ov );
    hudbar.bar_->setAnnotationFont( font );
    hudbar.bar_->setAnnotationTextSize( 14.f );
    const char32_t* expected[] = { U"0", U"25", U"50", U"75", U"100" };
    bool texts = hudbar.bar_->getNumLabels() == 5;
    for ( int idx=0; idx<5; idx++ )
	texts = texts && hudbar.bar_->labelText(idx)==expected[idx];

    mRunStandardTest( texts,
		      "Color bar labels: The labels are the values from the "
		      "start to the end of the range" );
    mRunStandardTest( ov.compile(), "Color bar labels: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels),
		      "Color bar labels: Image is read" );
    int nrmissing = 0;
    BufferString counts( "Pixels per label:" );
    const int labely = cBarBottom + cBarThickness*3/2;
    for ( int idx=0; idx<5; idx++ )
    {
	const int x = cBarLeft + idx*cBarLength/4;
	const int found = countWhite( pixels, x-12, x+12, labely, labely+16 );
	counts.add( " " ).add( found );
	if ( found < 5 )
	    nrmissing++;
    }

    mRunStandardTestWithError( nrmissing==0,
			       "Color bar labels: Every tick has a label above "
			       "it", counts );

    hudbar.bar_->setLabelFormatter(
	[]( float value ) { return "v" + std::to_string( int(value) ); } );
    hudbar.bar_->setRange( 10.f, 50.f );
    const char32_t* formatted[] = { U"v10", U"v20", U"v30", U"v40", U"v50" };
    texts = true;
    for ( int idx=0; idx<5; idx++ )
	texts = texts && hudbar.bar_->labelText(idx)==formatted[idx];

    mRunStandardTest( texts,
		      "Color bar labels: A formatter and a new range change "
		      "the labels" );

    hudbar.bar_->setNumLabels( 100 );
    const int most = hudbar.bar_->getNumLabels();
    hudbar.bar_->setNumLabels( 1 );
    const int least = hudbar.bar_->getNumLabels();
    mRunStandardTestWithError( most==vsgGeo::ScalarBar::cMaxLabels &&
			       least==2,
			       "Color bar labels: The number of labels is "
			       "between 2 and the most there is room for",
			       BufferString("Most: ",most).add(", least: ")
							  .add(least) );

    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Color bar labels: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testChanges()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(),
			       "Color bar changes: Off-screen viewer",
			       ov.errMsg().c_str() );

    HudBar hudbar( ov );
    mRunStandardTest( ov.compile(), "Color bar changes: Viewer compiled" );
    ov.frame( 3 );

    setColors( *hudbar.bar_, redToBlue(0.f) );
    ov.frame( 3 );
    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels),
		      "Color bar changes: Image is read" );
    const vsg::ubvec4 clear = pixelAt( pixels, cBarLeft+cBarLength/2,
				       cBarMiddle );
    mRunStandardTestWithError( clear.r<10 && clear.b<10,
			       "Color bar changes: A transparent color is not "
			       "drawn", describe(clear) );

    setColors( *hudbar.bar_, redToBlue() );
    hudbar.bar_->setPosition( vsg::vec2(float(cBarLeft),100.f) );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Color bar changes: Second image is read" );
    const vsg::ubvec4 oldplace = pixelAt( pixels, cBarLeft+cBarLength/2,
					  cBarMiddle );
    const vsg::ubvec4 newplace = pixelAt( pixels, cBarLeft+cBarLength/2,
					  100+cBarThickness/2 );
    mRunStandardTestWithError( oldplace.r<10 && oldplace.b<10 &&
			       newplace.r>100 && newplace.b>100,
			       "Color bar changes: A new position moves the "
			       "bar without compiling it again",
			       describe(oldplace).add(" and ")
						 .add(describe(newplace)) );

    hudbar.bar_->setAnnotationColor( vsg::vec4(0.f,1.f,0.f,1.f) );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Color bar changes: Third image is read" );
    const int newtop = 100 + cBarThickness;
    const vsg::ubvec4 border = pixelAt( pixels, cBarLeft+10, newtop );
    const vsg::ubvec4 border2 = pixelAt( pixels, cBarLeft+10, newtop-1 );
    mRunStandardTestWithError( (border.g>200 && border.r<60) ||
			       (border2.g>200 && border2.r<60),
			       "Color bar changes: The annotation color is the "
			       "color of the border", describe(border) );

    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Color bar changes: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testVertical()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Vertical bar:Font loaded" );

    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(),
			       "Vertical bar:Off-screen viewer",
			       ov.errMsg().c_str() );

    HudBar hudbar( ov );
    hudbar.bar_->setAnnotationFont( font );
    hudbar.bar_->setAnnotationTextSize( 14.f );
    makeVertical( *hudbar.bar_ );
    mRunStandardTest( hudbar.bar_->orientation() ==
				vsgGeo::ScalarBar::Orientation::Vertical,
		      "Vertical bar:The orientation is kept" );
    mRunStandardTest( ov.compile(), "Vertical bar:Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels),
		      "Vertical bar:Image is read" );
    const vsg::ubvec4 bottom = pixelAt( pixels, cVertMiddle, cVertBottom+5 );
    const vsg::ubvec4 middle = pixelAt( pixels, cVertMiddle,
					cVertBottom+cBarLength/2 );
    const vsg::ubvec4 top = pixelAt( pixels, cVertMiddle, cVertTop-5 );
    mRunStandardTestWithError( bottom.r>230 && bottom.b<30 && bottom.g<10,
			       "Vertical bar:The bar starts with the first "
			       "color at the bottom", describe(bottom) );
    mRunStandardTestWithError( std::abs(int(middle.r)-128)<20 &&
			       std::abs(int(middle.b)-127)<20 && middle.g<10,
			       "Vertical bar:The middle of the bar has the "
			       "color in the middle of the table",
			       describe(middle) );
    mRunStandardTestWithError( top.b>230 && top.r<30 && top.g<10,
			       "Vertical bar:The bar ends with the last "
			       "color at the top", describe(top) );
    const vsg::ubvec4 below = pixelAt( pixels, cVertMiddle, cVertBottom-10 );
    const vsg::ubvec4 above = pixelAt( pixels, cVertMiddle, cVertTop+10 );
    mRunStandardTestWithError( below.r<10 && below.b<10 &&
			       above.r<10 && above.b<10,
			       "Vertical bar:Nothing is drawn above or below "
			       "the bar", describe(below) );

    const int leftedge = countWhite( pixels, cVertLeft-1, cVertLeft+1,
				     cVertBottom+10, cVertTop-10 );
    const int rightedge = countWhite( pixels, cVertRight-1, cVertRight+1,
				      cVertBottom+10, cVertTop-10 );
    mRunStandardTestWithError( leftedge>=cBarLength-30 &&
			       rightedge>=cBarLength-30,
			       "Vertical bar:The bar has a white border "
			       "along its long sides",
			       BufferString("Left: ",leftedge).add(", right: ")
							      .add(rightedge) );

    const int nrticks = hudbar.bar_->getNumLabels();
    mRunStandardTestWithError( nrticks==5,
			       "Vertical bar:There are 5 labels at first",
			       BufferString("Labels: ",nrticks) );
    int nrmissingticks = 0;
    int nrmissinglabels = 0;
    BufferString counts( "Pixels per label:" );
    const int labelx = cVertLeft + cBarThickness*3/2 + 2;
    for ( int idx=0; idx<nrticks; idx++ )
    {
	const int y = cVertBottom + idx*cBarLength/(nrticks-1);
	const int tick = countWhite( pixels, cVertRight+3, cVertRight+8, y-1,
				     y+1 );
	if ( tick < 6 )
	    nrmissingticks++;

	const int label = countWhite( pixels, labelx, labelx+16, y-8, y+8 );
	counts.add( " " ).add( label );
	if ( label < 5 )
	    nrmissinglabels++;
    }

    mRunStandardTestWithError( nrmissingticks==0,
			       "Vertical bar:Every label has a tick right "
			       "of the bar", BufferString("Missing: ",
							  nrmissingticks) );
    mRunStandardTestWithError( nrmissinglabels==0,
			       "Vertical bar:Every tick has a label right "
			       "of it", counts );

    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Vertical bar:No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    vsgTest::installAssertTrace();

    if ( !testColors() ||
	 !testBorderAndTicks() ||
	 !testLabels() ||
	 !testChanges() ||
	 !testVertical() )
	return 1;

    return 0;
}
