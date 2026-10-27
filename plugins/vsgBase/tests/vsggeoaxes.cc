/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "vsggeoaxes.h"
#include "vsgtestassert.h"
#include "vsgtesthelper.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <filesystem>
#include <vector>

namespace
{

constexpr uint32_t cSize = 256;
constexpr double cCenterX = 128.;
constexpr double cCenterY = 128.;
constexpr double cArrowLength = 55.;
constexpr double cLabelDistance = 16.2;

using Pixels = TypeSet<vsg::ubvec4>;
using ColorTest = bool (*)( const vsg::ubvec4& );


vsg::ref_ptr<vsg::Font> loadFont()
{
    const std::filesystem::path dir =
	    std::filesystem::path( __FILE__ ).parent_path().parent_path();
    return vsg::read_cast<vsg::Font>(
		(dir / "folder_vsg" / "times.vsgb").string() );
}


bool isRed( const vsg::ubvec4& pix )
{
    return pix.r>100 && pix.g<60 && pix.b<60;
}


bool isGreen( const vsg::ubvec4& pix )
{
    return pix.g>100 && pix.r<60 && pix.b<60;
}


bool isBlue( const vsg::ubvec4& pix )
{
    return pix.b>100 && pix.r<60 && pix.g>=40 && pix.g<pix.b;
}


bool isYellow( const vsg::ubvec4& pix )
{
    return pix.r>100 && pix.g>100 && pix.b<60;
}


bool isWhite( const vsg::ubvec4& pix )
{
    return pix.r>200 && pix.g>200 && pix.b>200;
}


bool isModernRed( const vsg::ubvec4& pix )
{
    return pix.r>180 && pix.g>15 && pix.g<70 && pix.b>20 && pix.b<80;
}


bool isModernGreen( const vsg::ubvec4& pix )
{
    return pix.g>150 && pix.r>10 && pix.r<60 && pix.b>20 && pix.b<80;
}


bool isModernBlue( const vsg::ubvec4& pix )
{
    return pix.b>200 && pix.r>10 && pix.r<60 && pix.g>80 && pix.g<130;
}


bool isModernYellow( const vsg::ubvec4& pix )
{
    return pix.r>230 && pix.g>170 && pix.g<240 && pix.b>0 && pix.b<60;
}


bool isMagenta( const vsg::ubvec4& pix )
{
    return pix.r>200 && pix.g<80 && pix.b>200;
}


ColorTest modernColorTest( int dim )
{
    if ( dim == 0 )
	return isModernRed;

    if ( dim == 1 )
	return isModernGreen;

    return isModernBlue;
}


struct Spot
{
    int		nrpix_ = 0;
    double	x_ = 0.;
    double	y_ = 0.;
    int		minx_ = INT_MAX;
    int		maxx_ = -1;
    int		miny_ = INT_MAX;
    int		maxy_ = -1;

    bool	isNearCenter() const
		{
		    return nrpix_==0 || (std::abs(x_-cCenterX)<12. &&
					 std::abs(y_-cCenterY)<12.);
		}
};


Spot findSpot( const Pixels& pixels, ColorTest iscolor )
{
    Spot ret;
    double sumx = 0., sumy = 0.;
    for ( uint32_t y=0; y<cSize; y++ )
    {
	for ( uint32_t x=0; x<cSize; x++ )
	{
	    if ( !iscolor(pixels[int(y*cSize + x)]) )
		continue;

	    ret.nrpix_++;
	    sumx += x;
	    sumy += y;
	    ret.minx_ = std::min( ret.minx_, int(x) );
	    ret.maxx_ = std::max( ret.maxx_, int(x) );
	    ret.miny_ = std::min( ret.miny_, int(y) );
	    ret.maxy_ = std::max( ret.maxy_, int(y) );
	}
    }

    if ( ret.nrpix_ > 0 )
    {
	ret.x_ = sumx / ret.nrpix_;
	ret.y_ = sumy / ret.nrpix_;
    }

    return ret;
}


BufferString describe( const Spot& spot )
{
    BufferString ret( "(" );
    ret.add( spot.x_ ).add( "," ).add( spot.y_ ).add( ") with " )
       .add( spot.nrpix_ ).add( " pixels" );
    return ret;
}


struct HudAxes
{
		HudAxes( vsgTest::OffscreenViewer& ov,
			 vsgGeo::Updater* updater=nullptr,
			 vsgGeo::AxesNode::Style style=
					vsgGeo::AxesNode::Style::Classic )
		    : axes_(vsgGeo::AxesNode::create())
		{
		    vsgGeo::Text::setViewportHeight( float(ov.height()) );
		    ov.addHud()->addChild( axes_ );
		    axes_->setUpdater( updater );
		    axes_->setStyle( style );
		    axes_->setSize( vsg::vec2(5.f,float(cArrowLength)) );
		    axes_->setPosition( vsg::vec2(float(cCenterX),
						  float(cSize-cCenterY)) );
		    axes_->setPrimaryCamera(
				    vsg::ref_ptr<vsg::Camera>(&ov.camera()) );
		}

    vsg::ref_ptr<vsgGeo::AxesNode>	axes_;
};


vsg::dvec3 arrowDirection( int dim )
{
    if ( dim == 0 )
	return vsg::dvec3( 0., 1., 0. );

    if ( dim == 1 )
	return vsg::dvec3( 1., 0., 0. );

    return vsg::dvec3( 0., 0., -1. );
}


int nrNotYellowInDot( const Pixels& pixels )
{
    int ret = 0;
    for ( uint32_t y=0; y<cSize; y++ )
    {
	for ( uint32_t x=0; x<cSize; x++ )
	{
	    const double dist = std::hypot( double(x)+0.5-cCenterX,
					    double(y)+0.5-cCenterY );
	    if ( dist<3. && !isModernYellow(pixels[int(y*cSize + x)]) )
		ret++;
	}
    }

    return ret;
}


int countWhiteNear( const Pixels& pixels, double tipx, double tipy,
		    double offsetx, double offsety, double halfwidth=22. )
{
    int ret = 0;
    for ( uint32_t y=0; y<cSize; y++ )
    {
	for ( uint32_t x=0; x<cSize; x++ )
	{
	    if ( std::abs(double(x)-tipx-offsetx)<halfwidth &&
		 std::abs(double(y)-tipy+offsety)<halfwidth &&
		 isWhite(pixels[int(y*cSize + x)]) )
		ret++;
	}
    }

    return ret;
}


int countInColumn( const Pixels& pixels, ColorTest iscolor, uint32_t x )
{
    int ret = 0;
    for ( uint32_t y=0; y<cSize; y++ )
    {
	if ( iscolor(pixels[int(y*cSize + x)]) )
	    ret++;
    }

    return ret;
}


int countColorNear( const Pixels& pixels, ColorTest iscolor,
		    double centerx, double centery, double radius )
{
    int ret = 0;
    for ( uint32_t y=0; y<cSize; y++ )
    {
	for ( uint32_t x=0; x<cSize; x++ )
	{
	    if ( std::hypot(double(x)+0.5-centerx,
			    double(y)+0.5-centery)<radius &&
		 iscolor(pixels[int(y*cSize + x)]) )
		ret++;
	}
    }

    return ret;
}

} // namespace


static bool testOrientation()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(),
			       "Axes orientation: Off-screen viewer",
			       ov.errMsg().c_str() );

    HudAxes hudaxes( ov );
    mRunStandardTest( ov.compile(), "Axes orientation: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels),
		      "Axes orientation: Image is read" );
    const Spot ball = findSpot( pixels, isYellow );
    mRunStandardTestWithError( ball.nrpix_>20 &&
			       std::abs(ball.x_-cCenterX)<4. &&
			       std::abs(ball.y_-cCenterY)<4.,
			       "Axes orientation: The ball is at the "
			       "position, measured from the bottom left",
			       describe(ball) );

    Spot green = findSpot( pixels, isGreen );
    Spot blue = findSpot( pixels, isBlue );
    Spot red = findSpot( pixels, isRed );
    BufferString msg( "Green: " );
    msg.add( describe(green) ).add( ", blue: " ).add( describe(blue) )
       .add( ", red: " ).add( describe(red) );
    mRunStandardTestWithError( green.nrpix_>50 && green.x_>cCenterX+15. &&
			       std::abs(green.y_-cCenterY)<8.,
			       "Axes orientation: Looking along +Y, east "
			       "points right", msg );
    mRunStandardTestWithError( blue.nrpix_>50 && blue.y_>cCenterY+15. &&
			       std::abs(blue.x_-cCenterX)<8.,
			       "Axes orientation: Looking along +Y, down "
			       "points down", msg );
    mRunStandardTestWithError( red.isNearCenter(),
			       "Axes orientation: Looking along +Y, north "
			       "points away", msg );

    ov.camera().viewMatrix = vsg::LookAt::create(
		vsg::dvec3(0.,0.,5.), vsg::dvec3(0.,0.,0.),
		vsg::dvec3(0.,1.,0.) );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Axes orientation: Second image is read" );
    green = findSpot( pixels, isGreen );
    blue = findSpot( pixels, isBlue );
    red = findSpot( pixels, isRed );
    msg.set( "Green: " ).add( describe(green) ).add( ", blue: " )
       .add( describe(blue) ).add( ", red: " ).add( describe(red) );
    mRunStandardTestWithError( red.nrpix_>50 && red.y_<cCenterY-15. &&
			       std::abs(red.x_-cCenterX)<8.,
			       "Axes orientation: Looking down, north points "
			       "up", msg );
    mRunStandardTestWithError( green.nrpix_>50 && green.x_>cCenterX+15.,
			       "Axes orientation: Looking down, east points "
			       "right", msg );
    mRunStandardTestWithError( blue.isNearCenter(),
			       "Axes orientation: Looking down, down points "
			       "away", msg );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Axes orientation: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testOverScene()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(),
			       "Axes over a scene: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::AxesNode> scenenode = vsgGeo::AxesNode::create();
    scenenode->setSize( vsg::vec2(8.f,10.f) );
    vsg::ref_ptr<vsg::MatrixTransform> wall =
		vsg::MatrixTransform::create( vsg::translate(0.,3.,0.) );
    wall->addChild( scenenode );
    ov.scene().addChild( wall );

    HudAxes hudaxes( ov );
    mRunStandardTest( ov.compile(), "Axes over a scene: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels),
		      "Axes over a scene: Image is read" );
    const vsg::ubvec4 corner = pixels[10*cSize + 10];
    mRunStandardTestWithError( isYellow(corner),
			       "Axes over a scene: The scene is far in "
			       "front of the HUD",
			       BufferString("Corner pixel red: ",
					    int(corner.r)) );

    const Spot green = findSpot( pixels, isGreen );
    mRunStandardTestWithError( green.nrpix_>50 && green.x_>cCenterX+15.,
			       "Axes over a scene: The arrows are drawn over "
			       "the scene", describe(green) );
    return true;
}


static bool testNewSize()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Axes size: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    HudAxes hudaxes( ov, updater.get() );
    mRunStandardTest( ov.compile(), "Axes size: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Axes size: Image is read" );
    const Spot longarrow = findSpot( pixels, isGreen );
    const vsg::Node* firstgeometry = hudaxes.axes_->geometry();

    hudaxes.axes_->setSize( vsg::vec2(3.f,30.f) );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels), "Axes size: Second image is read" );
    const Spot shortarrow = findSpot( pixels, isGreen );
    BufferString msg( "Long: " );
    msg.add( describe(longarrow) ).add( ", short: " )
       .add( describe(shortarrow) );
    mRunStandardTest( hudaxes.axes_->geometry()!=firstgeometry,
		      "Axes size: A new size builds new geometry" );
    mRunStandardTestWithError( longarrow.nrpix_>50 && shortarrow.nrpix_>20 &&
			       shortarrow.x_-cCenterX <
					0.7*(longarrow.x_-cCenterX),
			       "Axes size: The arrows are shorter", msg );
    mRunStandardTest( updater->compiler().nrFailedCompiles()==0,
		      "Axes size: No failed compiles" );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Axes size: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testLabels()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Axes labels: Font loaded" );

    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Axes labels: Off-screen viewer",
			       ov.errMsg().c_str() );

    HudAxes hudaxes( ov );
    mRunStandardTestWithError( hudaxes.axes_->getAnnotationText(0)==U"N" &&
			       hudaxes.axes_->getAnnotationText(1)==U"E" &&
			       hudaxes.axes_->getAnnotationText(2)==U"Z",
			       "Axes labels: The labels are N, E and Z",
			       BufferString("Size: ",
				  hudaxes.axes_->getAnnotationTextSize()) );
    hudaxes.axes_->setAnnotationFont( font );
    hudaxes.axes_->setAnnotationTextSize( 40.f );
    mRunStandardTest( ov.compile(), "Axes labels: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Axes labels: Image is read" );
    int nrright = 0, nrbelow = 0, nrwhite = 0;
    for ( uint32_t y=0; y<cSize; y++ )
    {
	for ( uint32_t x=0; x<cSize; x++ )
	{
	    if ( !isWhite(pixels[int(y*cSize + x)]) )
		continue;

	    nrwhite++;
	    if ( double(x) > cCenterX+50. )
		nrright++;

	    if ( double(y) > cCenterY+40. )
		nrbelow++;
	}
    }

    BufferString msg( "White pixels: ", nrwhite );
    msg.add( ", right of the arrows: " ).add( nrright )
       .add( ", below them: " ).add( nrbelow );
    mRunStandardTestWithError( nrwhite>=20 && nrright>5 && nrbelow>5,
			       "Axes labels: The labels are at the tips of "
			       "the arrows", msg );

    hudaxes.axes_->setAnnotationTextSize( 80.f );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Axes labels: Second image is read" );
    int nrlarger = 0;
    for ( const vsg::ubvec4& pix : pixels )
    {
	if ( isWhite(pix) )
	    nrlarger++;
    }

    mRunStandardTestWithError( nrlarger > 2*nrwhite,
			       "Axes labels: A larger size gives larger "
			       "labels",
			       BufferString("White pixels: ",nrlarger) );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Axes labels: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testLabelsAllAngles( vsgGeo::AxesNode::Style style )
{
    const bool modern = style == vsgGeo::AxesNode::Style::Modern;
    const double labeloffset = 10.;
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Labels at all angles: Font loaded" );

    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(),
			       "Labels at all angles: Off-screen viewer",
			       ov.errMsg().c_str() );

    HudAxes hudaxes( ov, nullptr, style );
    hudaxes.axes_->setAnnotationFont( font );
    hudaxes.axes_->setAnnotationTextSize( 24.f );
    hudaxes.axes_->setAnnotationColor( vsg::vec4(1.f,1.f,1.f,1.f) );
    mRunStandardTest( ov.compile(), "Labels at all angles: Viewer compiled" );

    int minimum = INT_MAX;
    BufferString worst;
    const double torad = 3.14159265358979 / 180.;
    for ( int pitch=-60; pitch<=60; pitch+=30 )
    {
	for ( int yaw=0; yaw<360; yaw+=30 )
	{
	    const double pitchrad = pitch * torad;
	    const double yawrad = yaw * torad;
	    ov.camera().viewMatrix = vsg::LookAt::create(
		vsg::dvec3( 5.*std::cos(pitchrad)*std::sin(yawrad),
			    -5.*std::cos(pitchrad)*std::cos(yawrad),
			    5.*std::sin(pitchrad) ),
		vsg::dvec3(0.,0.,0.), vsg::dvec3(0.,0.,1.) );
	    ov.frame( 3 );

	    Pixels pixels;
	    if ( !ov.readColor(pixels) )
	    {
		mRunStandardTest( false,
				  "Labels at all angles: Image is read" );
	    }

	    const vsg::dmat4 view = ov.camera().viewMatrix->transform();
	    for ( int dim=0; dim<3; dim++ )
	    {
		const vsg::dvec3 tip = arrowDirection(dim) * cArrowLength;
		const vsg::dvec4 tipview = view * vsg::dvec4( tip, 0. );
		double offsetx = labeloffset, offsety = labeloffset;
		if ( modern )
		{
		    const double len = std::hypot( tipview.x, tipview.y );
		    offsetx = cLabelDistance *
			      ( len>1e-3 ? tipview.x/len : 0.7071 );
		    offsety = cLabelDistance *
			      ( len>1e-3 ? tipview.y/len : 0.7071 );
		}

		const int count = countWhiteNear( pixels, cCenterX+tipview.x,
						  cCenterY-tipview.y,
						  offsetx, offsety,
						  modern ? 14. : 22. );
		if ( count >= minimum )
		    continue;

		minimum = count;
		worst.set( "Label " ).add( dim ).add( " at pitch " )
		     .add( pitch ).add( " and yaw " ).add( yaw );
	    }
	}
    }

    BufferString msg( worst );
    msg.add( " has only " ).add( minimum ).add( " white pixels" );
    BufferString title( "Labels at all angles: All labels are drawn from "
			"every angle" );
    if ( modern )
	title.add( ", modern look" );

    mRunStandardTestWithError( minimum >= 8, title, msg );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Labels at all angles: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testModernLook()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Modern look: Font loaded" );

    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Modern look: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    HudAxes hudaxes( ov, updater.get(), vsgGeo::AxesNode::Style::Modern );
    mRunStandardTest( hudaxes.axes_->style()==vsgGeo::AxesNode::Style::Modern,
		      "Modern look: The style is modern" );
    hudaxes.axes_->setAnnotationFont( font );
    hudaxes.axes_->setAnnotationTextSize( 24.f );
    mRunStandardTestWithError( hudaxes.axes_->getMargin()>=cLabelDistance+12.,
			       "Modern look: The margin has room for the "
			       "labels",
			       BufferString("Margin: ",
					    hudaxes.axes_->getMargin()) );
    mRunStandardTest( ov.compile(), "Modern look: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Modern look: Image is read" );
    mRunStandardTestWithError(
		countWhiteNear(pixels,cCenterX+cArrowLength,cCenterY,
			       cLabelDistance,0.,14.)>=8 &&
		countWhiteNear(pixels,cCenterX,cCenterY+cArrowLength,
			       0.,-cLabelDistance,14.)>=8 &&
		countColorNear(pixels,isModernGreen,cCenterX+cArrowLength+
				cLabelDistance,cCenterY,14.)==0,
		"Modern look: The letters beyond the tips are white, as the "
		"letters of any other text", "No white letter pixels found" );
    const Spot dot = findSpot( pixels, isModernYellow );
    mRunStandardTestWithError( std::abs(dot.x_-cCenterX)<3. &&
			       std::abs(dot.y_-cCenterY)<3. &&
			       dot.nrpix_>30 && dot.nrpix_<90,
			       "Modern look: The yellow dot is small and at "
			       "the center", describe(dot) );
    mRunStandardTestWithError( nrNotYellowInDot(pixels)==0,
			       "Modern look: No arrow shows inside the yellow "
			       "dot",
			       BufferString("Pixels: ",
					    nrNotYellowInDot(pixels)) );

    hudaxes.axes_->setAnnotationColor( vsg::vec4(1.f,0.f,1.f,1.f) );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Modern look: Second image is read" );
    const Spot green = findSpot( pixels, isModernGreen );
    const Spot blue = findSpot( pixels, isModernBlue );
    BufferString msg( "Green: " );
    msg.add( describe(green) ).add( " from x " ).add( green.minx_ )
       .add( " to " ).add( green.maxx_ ).add( ", blue: " ).add( describe(blue) )
       .add( " from y " ).add( blue.miny_ ).add( " to " ).add( blue.maxy_ );
    mRunStandardTestWithError( green.nrpix_>60 &&
			       green.maxx_>=cCenterX+cArrowLength-4. &&
			       green.maxx_<=cCenterX+cArrowLength+1. &&
			       std::abs(green.y_-cCenterY)<3.,
			       "Modern look: The east arrow ends at the tip "
			       "of its line", msg );
    mRunStandardTestWithError( blue.nrpix_>60 &&
			       blue.maxy_>=cCenterY+cArrowLength-4. &&
			       blue.maxy_<=cCenterY+cArrowLength+1. &&
			       std::abs(blue.x_-cCenterX)<3.,
			       "Modern look: The arrow of down ends below the "
			       "center", msg );
    const int headwidth = countInColumn( pixels, isModernGreen,
					 uint32_t(cCenterX+cArrowLength-9.) );
    const int shaftwidth = countInColumn( pixels, isModernGreen,
					  uint32_t(cCenterX+cArrowLength/2.) );
    mRunStandardTestWithError( shaftwidth>=1 && shaftwidth<=3 &&
			       headwidth>=shaftwidth+2,
			       "Modern look: The arrow is a slim shaft with a "
			       "wider head",
			       BufferString("Head: ",headwidth)
					.add(", shaft: ").add(shaftwidth) );
    mRunStandardTestWithError(
		countColorNear(pixels,isMagenta,cCenterX+cArrowLength+
			       cLabelDistance,cCenterY,14.)>=8 &&
		countWhiteNear(pixels,cCenterX+cArrowLength,cCenterY,
			       cLabelDistance,0.,14.)==0,
		"Modern look: The annotation color is the color of the "
		"letters", "The letter beyond the tip is not magenta" );

    ov.camera().viewMatrix = vsg::LookAt::create(
		vsg::dvec3(0.,0.,5.), vsg::dvec3(0.,0.,0.),
		vsg::dvec3(0.,1.,0.) );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Modern look: Third image is read" );
    mRunStandardTestWithError( nrNotYellowInDot(pixels)==0,
			       "Modern look: No arrow shows inside the dot "
			       "from above",
			       BufferString("Pixels: ",
					    nrNotYellowInDot(pixels)) );
    const Spot red = findSpot( pixels, isModernRed );
    mRunStandardTestWithError( red.nrpix_>60 &&
			       red.miny_>=cCenterY-cArrowLength-1. &&
			       red.miny_<=cCenterY-cArrowLength+4. &&
			       std::abs(red.x_-cCenterX)<3.,
			       "Modern look: Looking down, the arrow of north "
			       "points up", describe(red) );

    hudaxes.axes_->setStyle( vsgGeo::AxesNode::Style::Classic );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Modern look: Fourth image is read" );
    mRunStandardTestWithError( findSpot(pixels,isModernRed).nrpix_==0 &&
			       findSpot(pixels,isRed).nrpix_>50,
			       "Modern look: The classic look comes back",
			       msg );
    mRunStandardTest( updater->compiler().nrFailedCompiles()==0,
		      "Modern look: No failed compiles" );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Modern look: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testNearerArrowHides()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Nearer arrow hides: Font loaded" );

    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(),
			       "Nearer arrow hides: Off-screen viewer",
			       ov.errMsg().c_str() );

    HudAxes hudaxes( ov, nullptr, vsgGeo::AxesNode::Style::Modern );
    hudaxes.axes_->setAnnotationFont( font );
    hudaxes.axes_->setAnnotationTextSize( 24.f );
    mRunStandardTest( ov.compile(), "Nearer arrow hides: Viewer compiled" );

    int neardim = -1, fardim = -1;
    double reach = 0.;
    const double torad = 3.14159265358979 / 180.;
    for ( int yaw=0; yaw<360 && neardim<0; yaw++ )
    {
	ov.camera().viewMatrix = vsg::LookAt::create(
		vsg::dvec3(5.*std::sin(yaw*torad),-5.*std::cos(yaw*torad),0.),
		vsg::dvec3(0.,0.,0.), vsg::dvec3(0.,0.,1.) );
	const vsg::dmat4 view = ov.camera().viewMatrix->transform();
	const vsg::dvec4 tipn = view * vsg::dvec4( 0., cArrowLength, 0., 0. );
	const vsg::dvec4 tipe = view * vsg::dvec4( cArrowLength, 0., 0., 0. );
	if ( std::abs(tipn.x-tipe.x)>2. || std::abs(tipn.y-tipe.y)>2. ||
	     std::abs(tipn.z-tipe.z)<60. )
	    continue;

	neardim = tipe.z > tipn.z ? 1 : 0;
	fardim = 1 - neardim;
	reach = std::hypot( tipe.x, tipe.y ) + 3.;
    }

    mRunStandardTest( neardim>=0,
		      "Nearer arrow hides: A view with two arrows on top of "
		      "each other exists" );
    ov.frame( 3 );
    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels),
		      "Nearer arrow hides: Image is read" );
    const int nrnear = countColorNear( pixels, modernColorTest(neardim),
				       cCenterX, cCenterY, reach );
    const int nrfar = countColorNear( pixels, modernColorTest(fardim),
				      cCenterX, cCenterY, reach );
    BufferString msg( "Pixels of the nearer arrow: " );
    msg.add( nrnear ).add( ", of the farther one: " ).add( nrfar )
       .add( " (arrow " ).add( fardim ).add( " is farther)" );
    mRunStandardTestWithError( nrnear>=40 && nrfar*10<=nrnear,
			       "Nearer arrow hides: What is behind the nearer "
			       "arrow is hidden", msg );

    ov.camera().viewMatrix = vsg::LookAt::create(
		vsg::dvec3(0.,0.,5.), vsg::dvec3(0.,0.,0.),
		vsg::dvec3(0.,1.,0.) );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Nearer arrow hides: Second image is read" );
    const int nrfarapart = countColorNear( pixels, modernColorTest(fardim),
					   cCenterX, cCenterY, 52. );
    mRunStandardTestWithError( nrfarapart>=40,
			       "Nearer arrow hides: The farther arrow shows "
			       "when it is not behind the other",
			       BufferString("Pixels: ",nrfarapart) );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    vsgTest::installAssertTrace();

    if ( !testOrientation() ||
	 !testOverScene() ||
	 !testNewSize() ||
	 !testLabels() ||
	 !testLabelsAllAngles(vsgGeo::AxesNode::Style::Classic) ||
	 !testLabelsAllAngles(vsgGeo::AxesNode::Style::Modern) ||
	 !testModernLook() ||
	 !testNearerArrowHides() )
	return 1;

    return 0;
}
