/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "visvsgdatagroup.h"
#include "visvsgtexturechannels.h"
#include "visvsgtexturerectangle.h"
#include "vsggeolayeredtexture.h"
#include "vsgtesthelper.h"

#include "coltabsequence.h"
#include "math2.h"

namespace
{

constexpr uint32_t cSize = 256;
constexpr int cTileSize = 5;
constexpr int cBigTileSize = 100;

using Pixels = TypeSet<vsg::ubvec4>;

struct Setup
{
    int		width_		= 7;
    int		height_		= 4;
    int		maxtile_	= cBigTileSize;
    bool	interpolate_	= false;
    Coord	shift_		= Coord( 0., 0. );
    Coord	growth_		= Coord( 0., 0. );
    bool	swapped_	= false;
    bool	rotated_	= false;
    bool	frozen_		= false;
};


float valueOf( const Setup& setup, int col, int row )
{
    return float( row*setup.width_ + col );
}


/*! Renders the image of values 0, 1, 2, ... with the colors going from red
    to blue, on a rectangle of two by two that fills the view. */

bool render( const Setup& setup, Pixels& pixels, Pixels* frozenpixels=nullptr )
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    if ( !ov.isOK() )
	return false;

    dGBVis::DataObject::setCommonViewer( &ov.viewer() );
    RefMan<dGBVis::DataObjectGroup> root = dGBVis::DataObjectGroup::create();
    ov.scene().addChild( vsg::ref_ptr<vsg::Node>(root->vsgNode()) );
    if ( !ov.compile() )
	return false;

    const int oldmax = vsgGeo::LayeredTexture::maxTextureSize();
    vsgGeo::LayeredTexture::setMaxTextureSize( setup.maxtile_ );
    const int nrvalues = setup.width_ * setup.height_;
    TypeSet<float> values;
    for ( int idx=0; idx<nrvalues; idx++ )
	values += float( idx );

    RefMan<dGBVis::TextureChannels> channels = dGBVis::TextureChannels::create();
    channels->enableTextureInterpolation( setup.interpolate_ );
    channels->setSize( 0, 1, setup.height_, setup.width_ );
    ColTab::MapperSetup mappersetup;
    mappersetup.type_ = ColTab::MapperSetup::Fixed;
    mappersetup.range_ = Interval<float>( 0.f, float(nrvalues-1) );
    channels->setColTabMapperSetup( 0, mappersetup );
    ColTab::Sequence seq;
    seq.setColor( 0.f, 255, 0, 0 );
    seq.setColor( 1.f, 0, 0, 255 );
    channels->setColTabSequence( seq );
    channels->setUnMappedData( 0, 0, values.arr(), OD::UsePtr, nullptr );
    vsgGeo::LayeredTexture::setMaxTextureSize( oldmax );

    RefMan<dGBVis::TextureRectangle> rectangle =
					dGBVis::TextureRectangle::create();
    rectangle->freezeDisplay( setup.frozen_ );
    rectangle->setTextureChannels( channels.ptr() );
    rectangle->setCenter( Coord3(0.,0.,0.) );
    rectangle->setWidth( Coord3(2.,0.,2.) );
    rectangle->setTextureShift( setup.shift_ );
    rectangle->setTextureGrowth( setup.growth_ );
    rectangle->swapTextureAxes( setup.swapped_ );
    if ( setup.rotated_ )
	rectangle->setRotation( Coord3(0.,0.,1.), Coord3(1.,0.,0.) );

    root->addObject( rectangle.ptr() );
    bool ok = true;
    if ( setup.frozen_ )
    {
	ov.frame( 3 );
	if ( frozenpixels )
	    ok = ov.readColor( *frozenpixels );

	rectangle->freezeDisplay( false );
    }

    ov.frame( 4 );
    ok = ok && ov.readColor( pixels );

    root = nullptr;
    ov.scene().children.clear();
    ov.viewer().deviceWaitIdle();
    dGBVis::DataObject::setCommonViewer( nullptr );
    return ok;
}


double cellFraction( int idx, int nrcells, double shift, double growth )
{
    const double size = ( nrcells>1 ? nrcells-1 : 1 ) + growth;
    const double start = 0.5*nrcells + shift - 0.5*size;
    const double lo = mMAX( 0., mMIN(1.,(idx-start)/size) );
    const double hi = mMAX( 0., mMIN(1.,(idx+1-start)/size) );
    return 0.5 * ( lo+hi );
}


bool isCellVisible( int idx, int nrcells, double shift, double growth )
{
    const double size = ( nrcells>1 ? nrcells-1 : 1 ) + growth;
    const double start = 0.5*nrcells + shift - 0.5*size;
    return (idx+1-start)/size > 0.05 && (idx-start)/size < 0.95;
}


/*! The pixel in the middle of the visible part of a cell. */

vsg::ubvec4 cellPixel( const Pixels& pixels, const Setup& setup, int col,
		       int row )
{
    const double fx = cellFraction( col, setup.width_, setup.shift_.x_,
				    setup.growth_.x_ );
    const double fy = cellFraction( row, setup.height_, setup.shift_.y_,
				    setup.growth_.y_ );
    const double fu = setup.swapped_ ? fy : fx;
    const double fv = setup.swapped_ ? fx : fy;
    const double fright = setup.rotated_ ? fv : fu;
    const double fup = setup.rotated_ ? fu : fv;
    const int x = mMIN( int(cSize)-1, int(fright*cSize) );
    const int ytop = mMIN( int(cSize)-1, int((1.-fup)*cSize) );
    return pixels[ ytop*int(cSize) + x ];
}


bool isExpectedColor( const vsg::ubvec4& pix, float value, int nrvalues )
{
    const int idx = mMIN( 254, int(value/float(nrvalues-1)*255.f) );
    const float pos = float(idx) / 254.f;
    return Math::Abs(int(pix.r)-int((1.f-pos)*255.f))<=8 &&
	   Math::Abs(int(pix.b)-int(pos*255.f))<=8 && pix.g<=8;
}


BufferString checkCells( const Pixels& pixels, const Setup& setup )
{
    BufferString wrong;
    for ( int row=0; row<setup.height_; row++ )
    {
	for ( int col=0; col<setup.width_; col++ )
	{
	    const bool visible =
		isCellVisible( col, setup.width_, setup.shift_.x_,
			       setup.growth_.x_ ) &&
		isCellVisible( row, setup.height_, setup.shift_.y_,
			       setup.growth_.y_ );
	    if ( !visible )
		continue;

	    const vsg::ubvec4 pix = cellPixel( pixels, setup, col, row );
	    if ( !isExpectedColor(pix,valueOf(setup,col,row),
				  setup.width_*setup.height_) )
	    {
		wrong.add( "Cell " ).add( col ).add( "," ).add( row ).add( ": (" )
		     .add( int(pix.r) ).add( "," ).add( int(pix.g) ).add( "," )
		     .add( int(pix.b) ).add( ") " );
	    }
	}
    }

    return wrong;
}

} // namespace


static bool testTiledAlignment()
{
    Setup setup;
    setup.maxtile_ = cTileSize;
    Pixels pixels;
    mRunStandardTest( render(setup,pixels), "Rectangle: Rendered in tiles" );
    const BufferString wrong = checkCells( pixels, setup );
    mRunStandardTestWithError( wrong.isEmpty(), "Rectangle: Every cell has its "
			       "color, with the corners at the middle of the "
			       "first and last cell", wrong );
    return true;
}


static bool testTilesLikeOne()
{
    for ( int interpolate=0; interpolate<2; interpolate++ )
    {
	Setup tiled;
	tiled.maxtile_ = cTileSize;
	tiled.interpolate_ = interpolate;
	Setup single = tiled;
	single.maxtile_ = cBigTileSize;
	Pixels tiledpixels, singlepixels;
	mRunStandardTest( render(tiled,tiledpixels) &&
			  render(single,singlepixels),
			  "Rectangle: Rendered with and without tiles" );
	int nrdifferent = 0;
	for ( int idx=0; idx<tiledpixels.size(); idx++ )
	{
	    const vsg::ubvec4& one = tiledpixels[idx];
	    const vsg::ubvec4& other = singlepixels[idx];
	    if ( Math::Abs(int(one.r)-int(other.r))>3 ||
		 Math::Abs(int(one.g)-int(other.g))>3 ||
		 Math::Abs(int(one.b)-int(other.b))>3 )
		nrdifferent++;
	}

	mRunStandardTestWithError( nrdifferent==0,
				   interpolate ? "Rectangle: Interpolated tiles "
					"look like one texture, without seams"
					: "Rectangle: Tiles look like one "
					  "texture",
				   BufferString("Different pixels: ",
						nrdifferent) );
    }

    return true;
}


static bool testShiftAndGrowth()
{
    Setup setup;
    setup.growth_ = Coord( 1., 0. );
    Pixels pixels;
    mRunStandardTest( render(setup,pixels), "Rectangle: Rendered with growth" );
    BufferString wrong = checkCells( pixels, setup );
    mRunStandardTestWithError( wrong.isEmpty(), "Rectangle: With a growth of "
			       "one the image fills the rectangle", wrong );

    setup.growth_ = Coord( 0., 0. );
    setup.shift_ = Coord( 1., -1. );
    mRunStandardTest( render(setup,pixels), "Rectangle: Rendered with shift" );
    wrong = checkCells( pixels, setup );
    mRunStandardTestWithError( wrong.isEmpty(), "Rectangle: A shift moves "
			       "the image over the rectangle", wrong );
    return true;
}


static bool testSwappedAndRotated()
{
    Setup setup;
    setup.swapped_ = true;
    Pixels pixels;
    mRunStandardTest( render(setup,pixels), "Rectangle: Rendered swapped" );
    BufferString wrong = checkCells( pixels, setup );
    mRunStandardTestWithError( wrong.isEmpty(), "Rectangle: Swapped texture "
			       "axes turn the image", wrong );

    setup.swapped_ = false;
    setup.rotated_ = true;
    mRunStandardTest( render(setup,pixels), "Rectangle: Rendered rotated" );
    wrong = checkCells( pixels, setup );
    mRunStandardTestWithError( wrong.isEmpty(), "Rectangle: A rotation puts "
			       "the rows along the second spanning vector",
			       wrong );

    setup.swapped_ = true;
    setup.maxtile_ = cTileSize;
    mRunStandardTest( render(setup,pixels), "Rectangle: Rendered rotated and "
					     "swapped in tiles" );
    wrong = checkCells( pixels, setup );
    mRunStandardTestWithError( wrong.isEmpty(), "Rectangle: Rotation, swapped "
			       "axes and tiles together", wrong );
    return true;
}


static bool testTransparentTable()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Transparent table: Off-screen "
			       "viewer", ov.errMsg().c_str() );

    dGBVis::DataObject::setCommonViewer( &ov.viewer() );
    RefMan<dGBVis::DataObjectGroup> root = dGBVis::DataObjectGroup::create();
    ov.scene().addChild( vsg::ref_ptr<vsg::Node>(root->vsgNode()) );
    mRunStandardTest( ov.compile(), "Transparent table: Viewer compiled" );

    RefMan<dGBVis::TextureChannels> channels = dGBVis::TextureChannels::create();
    channels->setSize( 0, 1, 2, 2 );
    ColTab::MapperSetup mappersetup;
    mappersetup.type_ = ColTab::MapperSetup::Fixed;
    mappersetup.range_ = Interval<float>( 0.f, 1.f );
    channels->setColTabMapperSetup( 0, mappersetup );
    ColTab::Sequence opaque;
    opaque.setColor( 0.f, 255, 255, 255 );
    opaque.setColor( 1.f, 255, 255, 255 );
    opaque.setUndefColor( OD::Color(255,255,255) );
    channels->setColTabSequence( opaque );
    const float values[] = { 0.f, 0.f, 0.f, 0.f };
    channels->setUnMappedData( 0, 0, values, OD::UsePtr, nullptr );
    mRunStandardTest( !channels->hasTransparency(),
		      "Transparent table: An opaque table has no transparency" );

    RefMan<dGBVis::TextureRectangle> rectangle =
					dGBVis::TextureRectangle::create();
    rectangle->setTextureChannels( channels.ptr() );
    rectangle->setCenter( Coord3(0.,0.,0.) );
    rectangle->setWidth( Coord3(2.,0.,2.) );
    root->addObject( rectangle.ptr() );
    ov.frame( 4 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Transparent table: Image is read" );
    const int centre = int(cSize/2)*int(cSize) + int(cSize/2);
    mRunStandardTestWithError( pixels[centre].g>250,
			       "Transparent table: An opaque table covers the "
			       "background",
			       BufferString("Green: ",int(pixels[centre].g)) );

    ColTab::Sequence transparent = opaque;
    transparent.setTransparency( Geom::Point2D<float>(0.f,128.f) );
    transparent.setTransparency( Geom::Point2D<float>(1.f,128.f) );
    channels->setColTabSequence( transparent );
    mRunStandardTest( channels->hasTransparency(),
		      "Transparent table: A table with transparency says so" );
    ov.frame( 4 );
    mRunStandardTest( ov.readColor(pixels),
		      "Transparent table: Image is read again" );
    mRunStandardTestWithError( Math::Abs(int(pixels[centre].g)-128)<12,
			       "Transparent table: A transparent table is "
			       "blended with the background",
			       BufferString("Green: ",int(pixels[centre].g)) );

    channels->setColTabSequence( opaque );
    ov.frame( 4 );
    mRunStandardTest( ov.readColor(pixels),
		      "Transparent table: Image is read once more" );
    mRunStandardTestWithError( pixels[centre].g>250,
			       "Transparent table: An opaque table again "
			       "covers the background",
			       BufferString("Green: ",int(pixels[centre].g)) );

    root = nullptr;
    ov.scene().children.clear();
    ov.viewer().deviceWaitIdle();
    dGBVis::DataObject::setCommonViewer( nullptr );
    return true;
}


static bool testFrozen()
{
    Setup setup;
    setup.frozen_ = true;
    Pixels frozenpixels, pixels;
    mRunStandardTest( render(setup,pixels,&frozenpixels),
		      "Rectangle: Rendered frozen and unfrozen" );
    bool allblack = true;
    for ( const vsg::ubvec4& pix : frozenpixels )
    {
	if ( pix.r>10 || pix.g>10 || pix.b>10 )
	    allblack = false;
    }

    mRunStandardTest( allblack, "Rectangle: A frozen rectangle shows nothing "
				"of its changes" );
    const BufferString wrong = checkCells( pixels, setup );
    mRunStandardTestWithError( wrong.isEmpty(), "Rectangle: Unfreezing shows "
			       "the changes", wrong );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testTiledAlignment() ||
	 !testTilesLikeOne() ||
	 !testShiftAndGrowth() ||
	 !testSwappedAndRotated() ||
	 !testTransparentTable() ||
	 !testFrozen() )
	return 1;

    return 0;
}
