/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "vsggeotextureplane.h"
#include "vsgtesthelper.h"

#include "math2.h"

namespace
{

constexpr uint32_t cSize = 256;
constexpr int cNrColumns = 4;
constexpr int cNrRows = 2;
constexpr int cCellWidth = cSize / cNrColumns;
constexpr int cCellHeight = cSize / cNrRows;
constexpr unsigned char cUdfIdx = 255;

using Pixels = TypeSet<vsg::ubvec4>;


void setRedToBlue( vsgGeo::LayeredTexture& texture )
{
    const int nrcolors = texture.colorLookup().nrColors();
    TypeSet<vsg::vec4> colors( nrcolors, vsg::vec4() );
    for ( int idx=0; idx<nrcolors; idx++ )
    {
	const float level = float(idx) / float(nrcolors-1);
	colors[idx] = vsg::vec4( 1.f-level, 0.f, level, 1.f );
    }

    texture.colorLookup().setColors( colors.arr(), nrcolors,
				     vsg::vec4(0.f,1.f,0.f,1.f) );
}


void setUniformColor( vsgGeo::LayeredTexture& texture, const vsg::vec4& color,
		      const vsg::vec4& undefcolor )
{
    const int nrcolors = texture.colorLookup().nrColors();
    const TypeSet<vsg::vec4> colors( nrcolors, color );
    texture.colorLookup().setColors( colors.arr(), nrcolors, undefcolor );
}


bool setIndices( vsgGeo::LayeredTexture& texture,
		 const unsigned char* indices, int nrindices )
{
    TypeSet<unsigned char> bands;
    for ( int idx=0; idx<nrindices; idx++ )
    {
	const unsigned char defined = indices[idx]==cUdfIdx ? 0 : 255;
	bands += indices[idx];
	bands += defined;
    }

    return texture.setIndices( bands.arr(), size_t(bands.size()) );
}


template <size_t N>
bool setIndices( vsgGeo::LayeredTexture& texture,
		 const unsigned char (&indices)[N] )
{
    return setIndices( texture, indices, int(N) );
}


vsg::ubvec4 pixelAt( const Pixels& pixels, double xfrac, double yfrac )
{
    return pixels[ int(yfrac*cSize)*int(cSize) + int(xfrac*cSize) ];
}


vsg::ubvec4 cellColor( const Pixels& pixels, int col, int row )
{
    const int x = col*cCellWidth + cCellWidth/2;
    const int ytop = (cNrRows-1-row)*cCellHeight + cCellHeight/2;
    return pixels[ int(ytop*cSize + x) ];
}


BufferString describe( const vsg::ubvec4& pix )
{
    BufferString ret( "(" );
    ret.add( int(pix.r) ).add( "," ).add( int(pix.g) ).add( "," )
       .add( int(pix.b) ).add( ")" );
    return ret;
}


bool isRed( const vsg::ubvec4& pix )
{
    return pix.r>230 && pix.g<10 && pix.b<30;
}


bool isBlue( const vsg::ubvec4& pix )
{
    return pix.b>230 && pix.g<10 && pix.r<30;
}


bool isGreen( const vsg::ubvec4& pix )
{
    return pix.g>230 && pix.r<10 && pix.b<10;
}


bool isMiddle( const vsg::ubvec4& pix )
{
    return Math::Abs(int(pix.r)-128)<20 && Math::Abs(int(pix.b)-127)<20 &&
	   pix.g<10;
}

} // namespace


static bool testSetIndices()
{
    vsg::ref_ptr<vsgGeo::LayeredTexture> texture =
		vsgGeo::LayeredTexture::create( cNrColumns, cNrRows );
    mRunStandardTest( texture->width()==cNrColumns &&
		      texture->height()==cNrRows, "Texture: Image size" );
    mRunStandardTest( texture->isUndefAt(0,0),
		      "Texture: A new image is undefined" );

    const unsigned char indices[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    mRunStandardTest( setIndices(*texture,indices), "Texture: Indices set" );
    mRunStandardTest( texture->indexAt(0,0)==0 && texture->indexAt(3,0)==3 &&
		      texture->indexAt(0,1)==4 && texture->indexAt(3,1)==7,
		      "Texture: The first row comes first" );
    const unsigned char wrongnr[5] = { 0, 0, 0, 0, 0 };
    mRunStandardTest( !setIndices(*texture,wrongnr),
		      "Texture: A wrong number of indices is refused" );
    return true;
}


static bool testRendering()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Texture plane: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::LayeredTexture> texture =
		vsgGeo::LayeredTexture::create( cNrColumns, cNrRows );
    setRedToBlue( *texture );
    const unsigned char indices[] = { 0, 127, 254, cUdfIdx,
				      254, 254, 0, 0 };
    mRunStandardTest( setIndices(*texture,indices),
		      "Texture plane: Indices set" );

    ov.scene().addChild( vsgGeo::TexturePlane::create( texture,
		vsg::vec3(-1.f,0.f,-1.f), vsg::vec3(2.f,0.f,0.f),
		vsg::vec3(0.f,0.f,2.f) ) );
    mRunStandardTest( ov.compile(), "Texture plane: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Texture plane: Image is read" );
    mRunStandardTestWithError( isRed(cellColor(pixels,0,0)),
			       "Texture plane: Index 0 is the first color",
			       describe(cellColor(pixels,0,0)) );
    mRunStandardTestWithError( isMiddle(cellColor(pixels,1,0)),
			       "Texture plane: Index 127 is the middle color",
			       describe(cellColor(pixels,1,0)) );
    mRunStandardTestWithError( isBlue(cellColor(pixels,2,0)),
			       "Texture plane: Index 254 is the last color",
			       describe(cellColor(pixels,2,0)) );
    mRunStandardTestWithError( isGreen(cellColor(pixels,3,0)),
			       "Texture plane: Index 255 is the undefined "
			       "color", describe(cellColor(pixels,3,0)) );
    mRunStandardTestWithError( isBlue(cellColor(pixels,0,1)) &&
			       isRed(cellColor(pixels,2,1)),
			       "Texture plane: The second row is on top",
			       describe(cellColor(pixels,0,1)) );

    const unsigned char changed[] = { 254, 254, 254, 254,
				      0, 0, 0, 0 };
    mRunStandardTest( setIndices(*texture,changed),
		      "Texture plane: Indices changed" );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Texture plane: Changed image is read" );
    mRunStandardTestWithError( isBlue(cellColor(pixels,0,0)) &&
			       isRed(cellColor(pixels,0,1)),
			       "Texture plane: New indices are shown",
			       describe(cellColor(pixels,0,0)) );

    setUniformColor( *texture, vsg::vec4(1.f,1.f,0.f,1.f),
		     vsg::vec4(0.f,1.f,0.f,1.f) );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Texture plane: Recolored image is read" );
    const vsg::ubvec4 yellow = cellColor( pixels, 0, 0 );
    mRunStandardTestWithError( yellow.r>230 && yellow.g>230 && yellow.b<10,
			       "Texture plane: A new color table is shown "
			       "without new indices", describe(yellow) );

    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Texture plane: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testInterpolation()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Interpolation: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::LayeredTexture> texture =
		vsgGeo::LayeredTexture::create( 2, 2, nullptr, true );
    setRedToBlue( *texture );
    mRunStandardTest( texture->isInterpolated(),
		      "Interpolation: The texture is interpolated" );
    const unsigned char indices[] = { 0, 254, 0, 254 };
    mRunStandardTest( setIndices(*texture,indices),
		      "Interpolation: Indices set" );
    ov.scene().addChild( vsgGeo::TexturePlane::create( texture,
		vsg::vec3(-1.f,0.f,-1.f), vsg::vec3(2.f,0.f,0.f),
		vsg::vec3(0.f,0.f,2.f) ) );
    mRunStandardTest( ov.compile(), "Interpolation: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Interpolation: Image is read" );
    const vsg::ubvec4 first = pixelAt( pixels, 0.25, 0.5 );
    const vsg::ubvec4 middle = pixelAt( pixels, 0.5, 0.5 );
    const vsg::ubvec4 second = pixelAt( pixels, 0.75, 0.5 );
    mRunStandardTestWithError( isRed(first) && isBlue(second),
			       "Interpolation: The middle of a cell has the "
			       "color of its value", describe(first) );
    mRunStandardTestWithError( Math::Abs(int(middle.r)-128)<14 &&
			       Math::Abs(int(middle.b)-127)<14 && middle.g<10,
			       "Interpolation: Between two cells the colors "
			       "blend", describe(middle) );
    return true;
}


static bool testInterpolationAtUndefined()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Undefined: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::LayeredTexture> texture =
		vsgGeo::LayeredTexture::create( 2, 1, nullptr, true );
    setRedToBlue( *texture );
    const unsigned char indices[] = { 0, cUdfIdx };
    mRunStandardTest( setIndices(*texture,indices),
		      "Undefined: Indices set" );
    ov.scene().addChild( vsgGeo::TexturePlane::create( texture,
		vsg::vec3(-1.f,0.f,-1.f), vsg::vec3(2.f,0.f,0.f),
		vsg::vec3(0.f,0.f,2.f) ) );
    mRunStandardTest( ov.compile(), "Undefined: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Undefined: Image is read" );
    const vsg::ubvec4 besidepix = pixelAt( pixels, 0.4, 0.5 );
    const vsg::ubvec4 insidepix = pixelAt( pixels, 0.9, 0.5 );
    mRunStandardTestWithError( isRed(besidepix),
			       "Undefined: Next to an undefined cell the color "
			       "is still that of the defined one",
			       describe(besidepix) );
    mRunStandardTestWithError( isGreen(insidepix), "Undefined: In the "
			       "undefined cell the undefined color is shown",
			       describe(insidepix) );
    return true;
}


static vsg::ref_ptr<vsg::StateGroup> stateGroupOf(
					const vsgGeo::TexturePlane& plane )
{
    vsg::ref_ptr<vsg::DepthSorted> sorted =
			plane.children.front().cast<vsg::DepthSorted>();
    return sorted ? sorted->child.cast<vsg::StateGroup>()
		  : plane.children.front().cast<vsg::StateGroup>();
}


static bool testSharedPipeline()
{
    vsg::ref_ptr<vsg::SharedObjects> shared = vsg::SharedObjects::create();
    TypeSet<vsg::ref_ptr<vsgGeo::TexturePlane>> planes;
    for ( int idx=0; idx<2; idx++ )
    {
	planes += vsgGeo::TexturePlane::create(
		vsgGeo::LayeredTexture::create(2,2), vsg::vec3(0.f,0.f,0.f),
		vsg::vec3(1.f,0.f,0.f), vsg::vec3(0.f,0.f,1.f),
		vsg::vec2(0.f,0.f), vsg::vec2(1.f,0.f), vsg::vec2(0.f,1.f),
		shared );
    }

    vsg::ref_ptr<vsg::StateGroup> first = stateGroupOf( *planes[0] );
    vsg::ref_ptr<vsg::StateGroup> second = stateGroupOf( *planes[1] );
    mRunStandardTest( first && second && !first->stateCommands.empty() &&
		      first->stateCommands.front() ==
				second->stateCommands.front(),
		      "Shared pipeline: Planes with shared objects use the "
		      "same pipeline" );
    return true;
}


vsg::ref_ptr<vsgGeo::LayeredTexture> makeWhiteTexture( float alpha )
{
    vsg::ref_ptr<vsgGeo::LayeredTexture> texture =
		vsgGeo::LayeredTexture::create( 2, 2 );
    setUniformColor( *texture, vsg::vec4(1.f,1.f,1.f,alpha),
		     vsg::vec4(1.f,1.f,1.f,alpha) );
    const unsigned char indices[] = { 0, 0, 0, 0 };
    setIndices( *texture, indices );
    return texture;
}


static bool testNoBlending()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "No blending: Off-screen viewer",
			       ov.errMsg().c_str() );

    ov.scene().addChild( vsgGeo::TexturePlane::create( makeWhiteTexture(0.5f),
		vsg::vec3(-1.f,0.f,-1.f), vsg::vec3(2.f,0.f,0.f),
		vsg::vec3(0.f,0.f,2.f), vsg::vec2(0.f,0.f), vsg::vec2(1.f,0.f),
		vsg::vec2(0.f,1.f), nullptr, false ) );
    mRunStandardTest( ov.compile(), "No blending: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "No blending: Image is read" );
    const vsg::ubvec4 pix = pixelAt( pixels, 0.5, 0.5 );
    mRunStandardTestWithError( pix.g>250, "No blending: The transparency of "
			       "the color is not used", describe(pix) );
    return true;
}


static bool testBlendedPlaneKeepsDepth()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Depth: Off-screen viewer",
			       ov.errMsg().c_str() );

    ov.scene().addChild( vsgGeo::TexturePlane::create( makeWhiteTexture(0.f),
		vsg::vec3(-1.f,0.f,-1.f), vsg::vec3(2.f,0.f,0.f),
		vsg::vec3(0.f,0.f,2.f) ) );
    ov.scene().addChild( vsgGeo::TexturePlane::create( makeWhiteTexture(1.f),
		vsg::vec3(-1.f,1.f,-1.f), vsg::vec3(2.f,0.f,0.f),
		vsg::vec3(0.f,0.f,2.f), vsg::vec2(0.f,0.f), vsg::vec2(1.f,0.f),
		vsg::vec2(0.f,1.f), nullptr, false ) );
    mRunStandardTest( ov.compile(), "Depth: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Depth: Image is read" );
    const vsg::ubvec4 pix = pixelAt( pixels, 0.5, 0.5 );
    mRunStandardTestWithError( pix.g>250, "Depth: What is drawn after a "
			       "transparent plane shows through it",
			       describe(pix) );
    return true;
}


vsg::ref_ptr<vsgGeo::LayeredTexture> makeColorTexture( float red, float blue )
{
    vsg::ref_ptr<vsgGeo::LayeredTexture> texture =
		vsgGeo::LayeredTexture::create( 2, 2 );
    setUniformColor( *texture, vsg::vec4(red,0.f,blue,0.5f),
		     vsg::vec4(red,0.f,blue,0.5f) );
    const unsigned char indices[] = { 0, 0, 0, 0 };
    setIndices( *texture, indices );
    return texture;
}


static bool testOpaqueCellsHideWhatIsBehind()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Hiding: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::LayeredTexture> red =
		vsgGeo::LayeredTexture::create( 2, 2 );
    setUniformColor( *red, vsg::vec4(1.f,0.f,0.f,1.f),
		     vsg::vec4(1.f,1.f,1.f,0.f) );
    const unsigned char redindices[] = { 0, 0, 0, 255 };
    setIndices( *red, redindices );

    ov.scene().addChild( vsgGeo::TexturePlane::create( red,
		vsg::vec3(-1.f,0.f,-1.f), vsg::vec3(2.f,0.f,0.f),
		vsg::vec3(0.f,0.f,2.f) ) );
    ov.scene().addChild( vsgGeo::TexturePlane::create( makeWhiteTexture(1.f),
		vsg::vec3(-1.f,1.f,-1.f), vsg::vec3(2.f,0.f,0.f),
		vsg::vec3(0.f,0.f,2.f), vsg::vec2(0.f,0.f), vsg::vec2(1.f,0.f),
		vsg::vec2(0.f,1.f), nullptr, false ) );
    mRunStandardTest( ov.compile(), "Hiding: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Hiding: Image is read" );
    const vsg::ubvec4 opaque = pixelAt( pixels, 0.25, 0.25 );
    const vsg::ubvec4 clear = pixelAt( pixels, 0.9, 0.1 );
    mRunStandardTestWithError( isRed(opaque), "Hiding: An opaque cell of a "
			       "plane with transparent colors hides what is "
			       "behind it", describe(opaque) );
    mRunStandardTestWithError( clear.g>250 && clear.b>250,
			       "Hiding: A transparent cell shows what is "
			       "behind it", describe(clear) );
    return true;
}


static bool testTransparentOrder()
{
    for ( int nearfirst=0; nearfirst<2; nearfirst++ )
    {
	vsgTest::OffscreenViewer ov( cSize, cSize );
	mRunStandardTestWithError( ov.isOK(), "Order: Off-screen viewer",
				   ov.errMsg().c_str() );

	vsg::ref_ptr<vsgGeo::TexturePlane> nearplane =
	    vsgGeo::TexturePlane::create( makeColorTexture(0.f,1.f),
		vsg::vec3(-1.f,0.f,-1.f), vsg::vec3(2.f,0.f,0.f),
		vsg::vec3(0.f,0.f,2.f) );
	vsg::ref_ptr<vsgGeo::TexturePlane> farplane =
	    vsgGeo::TexturePlane::create( makeColorTexture(1.f,0.f),
		vsg::vec3(-1.f,1.f,-1.f), vsg::vec3(2.f,0.f,0.f),
		vsg::vec3(0.f,0.f,2.f) );
	if ( nearfirst )
	{
	    ov.scene().addChild( nearplane );
	    ov.scene().addChild( farplane );
	}
	else
	{
	    ov.scene().addChild( farplane );
	    ov.scene().addChild( nearplane );
	}

	mRunStandardTest( ov.compile(), "Order: Viewer compiled" );
	ov.frame( 3 );

	Pixels pixels;
	mRunStandardTest( ov.readColor(pixels), "Order: Image is read" );
	const vsg::ubvec4 pix = pixelAt( pixels, 0.5, 0.5 );
	mRunStandardTestWithError( Math::Abs(int(pix.r)-64)<12 && pix.g<6 &&
				   Math::Abs(int(pix.b)-128)<12,
				   nearfirst ? "Order: Transparent planes are "
				   "blended back to front when the near one "
				   "comes first" : "Order: Transparent planes "
				   "are blended back to front when the far one "
				   "comes first", describe(pix) );
    }

    return true;
}


static bool testTransparency()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Transparency: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::LayeredTexture> texture =
		vsgGeo::LayeredTexture::create( cNrColumns, cNrRows );
    const int nrcolors = texture->colorLookup().nrColors();
    TypeSet<vsg::vec4> colors( nrcolors, vsg::vec4(1.f,1.f,1.f,1.f) );
    colors[1] = vsg::vec4( 1.f, 1.f, 1.f, 0.5f );
    colors[2] = vsg::vec4( 1.f, 1.f, 1.f, 0.f );
    texture->colorLookup().setColors( colors.arr(), nrcolors,
				      vsg::vec4(1.f,1.f,1.f,0.25f) );
    const unsigned char indices[] = { 0, 1, 2, cUdfIdx,
				      0, 1, 2, cUdfIdx };
    mRunStandardTest( setIndices(*texture,indices),
		      "Transparency: Indices set" );

    ov.scene().addChild( vsgGeo::TexturePlane::create( texture,
		vsg::vec3(-1.f,0.f,-1.f), vsg::vec3(2.f,0.f,0.f),
		vsg::vec3(0.f,0.f,2.f) ) );
    mRunStandardTest( ov.compile(), "Transparency: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Transparency: Image is read" );
    const vsg::ubvec4 opaque = cellColor( pixels, 0, 0 );
    const vsg::ubvec4 half = cellColor( pixels, 1, 0 );
    const vsg::ubvec4 clear = cellColor( pixels, 2, 0 );
    const vsg::ubvec4 udf = cellColor( pixels, 3, 0 );
    mRunStandardTestWithError( opaque.g>250, "Transparency: An opaque color "
			       "covers the background", describe(opaque) );
    mRunStandardTestWithError( Math::Abs(int(half.g)-128)<12,
			       "Transparency: A half transparent color is "
			       "blended with the background", describe(half) );
    mRunStandardTestWithError( clear.g<6, "Transparency: A fully transparent "
			       "color shows the background", describe(clear) );
    mRunStandardTestWithError( Math::Abs(int(udf.g)-64)<12,
			       "Transparency: The undefined color is blended "
			       "too", describe(udf) );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Transparency: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testSetIndices() ||
	 !testRendering() ||
	 !testInterpolation() ||
	 !testInterpolationAtUndefined() ||
	 !testSharedPipeline() ||
	 !testNoBlending() ||
	 !testBlendedPlaneKeepsDepth() ||
	 !testOpaqueCellsHideWhatIsBehind() ||
	 !testTransparentOrder() ||
	 !testTransparency() )
	return 1;

    return 0;
}
