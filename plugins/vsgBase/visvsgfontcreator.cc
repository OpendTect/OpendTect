/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgfontcreator.h"

#include "bufstring.h"
#include "bufstringset.h"
#include "visvsgdata.h"
#include "manobjectset.h"
#include "math2.h"
#include "perthreadrepos.h"
#include "thread.h"
#include "typeset.h"

#include <vsg/text/GlyphMetrics.h>

#ifdef __win__
# include <windows.h>
# include <dwrite.h>
# pragma comment(lib, "dwrite.lib")
#elif defined(__unix__) && !defined(__mac__)
# include <fontconfig/fontconfig.h>
# include <ft2build.h>
# include FT_FREETYPE_H
#endif

#include <limits>

namespace dGBVis
{

namespace
{

#if defined(__win__) || (defined(__unix__) && !defined(__mac__))

constexpr float cEmSize = 48.f;
constexpr int cMargin = 6;
constexpr uint32_t cFirstChar = 32;
constexpr uint32_t cLastChar = 126;


struct GlyphBitmap
{
    uint32_t		charcode	= 0;
    int			width		= 0;
    int			height		= 0;
    int			originx		= 0;
    int			originy		= 0;
    float		horiAdvance	= 0.f;
    float		vertBearingX	= 0.f;
    float		vertBearingY	= 0.f;
    float		vertAdvance	= 0.f;
    TypeSet<od_int16>	sdf;

    bool		hasBitmap() const
			{ return width>0 && height>0 && !sdf.isEmpty(); }
};


#ifdef __win__

template <typename T>
struct DirectWriteCOMPtr
{
    T*		ptr_			= nullptr;

		~DirectWriteCOMPtr()	{ if ( ptr_ ) ptr_->Release(); }

    T**		operator&()		{ return &ptr_; }
    T*		get() const		{ return ptr_; }
    T*		operator->() const	{ return ptr_; }
    explicit	operator bool() const	{ return ptr_ != nullptr; }
};


bool openDirectWriteFace( const BufferString& family, bool bold, bool italic,
			  DirectWriteCOMPtr<IDWriteFactory>& factory,
			  DirectWriteCOMPtr<IDWriteFontFace>& face )
{
    if ( FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
				    __uuidof(IDWriteFactory),
				    reinterpret_cast<IUnknown**>(&factory))) )
	return false;

    DirectWriteCOMPtr<IDWriteFontCollection> collection;
    if ( FAILED(factory->GetSystemFontCollection(&collection)) )
	return false;

    UINT32 familyindex = 0;
    BOOL familyexists = FALSE;
    const std::wstring wfamily = family.toStdWString();
    if ( FAILED(collection->FindFamilyName(wfamily.c_str(),
					   &familyindex,&familyexists)) ||
	 !familyexists )
	return false;

    DirectWriteCOMPtr<IDWriteFontFamily> fontfamily;
    if ( FAILED(collection->GetFontFamily(familyindex,&fontfamily)) )
	return false;

    const DWRITE_FONT_WEIGHT weight =
		bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
    const DWRITE_FONT_STYLE style =
		italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

    DirectWriteCOMPtr<IDWriteFont> font;
    if ( FAILED(fontfamily->GetFirstMatchingFont(weight,
						 DWRITE_FONT_STRETCH_NORMAL,
						 style,&font)) )
	return false;

    return SUCCEEDED( font->CreateFontFace(&face) );
}

#endif


bool isInsideGlyph( const TypeSet<od_uint8>& alpha, int width, int height,
		    int margin, int x, int y )
{
    const int gx = x - margin;
    const int gy = y - margin;
    if ( gx < 0 || gy < 0 || gx >= width || gy >= height )
	return false;

    return alpha[gy*width + gx] > 127;
}


void makeSdfFromAlpha( const TypeSet<od_uint8>& alpha,
		       int width, int height, int margin,
		       TypeSet<od_int16>& sdf )
{
    const int outw = width + 2 * margin;
    const int outh = height + 2 * margin;
    sdf.setSize( outw*outh, 0 );

    const float maxdist = float( margin );
    for ( int y=0; y<outh; ++y )
    {
	for ( int x=0; x<outw; ++x )
	{
	    const bool inside = isInsideGlyph( alpha, width, height, margin,
					       x, y );
	    float bestdist = maxdist;
	    const int r0 = mMAX( 0, y - margin );
	    const int r1 = mMIN( outh - 1, y + margin );
	    const int c0 = mMAX( 0, x - margin );
	    const int c1 = mMIN( outw - 1, x + margin );
	    for ( int ry=r0; ry<=r1; ++ry )
	    {
		for ( int rx=c0; rx<=c1; ++rx )
		{
		    if ( isInsideGlyph(alpha,width,height,margin,rx,ry)
								== inside )
			continue;

		    const float dx = float( rx - x );
		    const float dy = float( ry - y );
		    const float dist = Math::Sqrt( dx*dx + dy*dy );
		    if ( dist < bestdist )
			bestdist = dist;
		}
	    }

	    float signeddist = inside ? bestdist : -bestdist;
	    signeddist = mMAX( -1.f, mMIN(1.f,signeddist/maxdist) );
	    sdf[y*outw + x] = od_int16( signeddist *
			     float(std::numeric_limits<od_int16>::max()) );
	}
    }
}


vsg::ref_ptr<vsg::Font> assembleFont(
			const ManagedObjectSet<GlyphBitmap>& glyphs,
			float ascender, float descender, float height )
{
    if ( glyphs.isEmpty() )
	return {};

    const int atlaswidth = 512;
    int xpos = cMargin;
    int ypos = cMargin;
    int rowheight = 0;
    int neededheight = cMargin;
    for ( const GlyphBitmap* glyph : glyphs )
    {
	const GlyphBitmap& gb = *glyph;
	if ( !gb.hasBitmap() )
	    continue;

	const int cellw = gb.width + 2 * cMargin;
	const int cellh = gb.height + 2 * cMargin;
	if ( xpos + cellw + cMargin > atlaswidth )
	{
	    xpos = cMargin;
	    ypos += rowheight + cMargin;
	    rowheight = 0;
	}

	xpos += cellw + cMargin;
	rowheight = mMAX( rowheight, cellh );
	neededheight = mMAX( neededheight, ypos + cellh + cMargin );
    }

    int atlasheight = 64;
    while ( atlasheight < neededheight )
	atlasheight *= 2;

    vsg::ref_ptr<vsg::shortArray2D> atlas = vsg::shortArray2D::create(
				uint32_t(atlaswidth), uint32_t(atlasheight),
				vsg::Data::Properties{VK_FORMAT_R16_SNORM} );
    for ( auto& cell : *atlas )
	cell = std::numeric_limits<int16_t>::lowest();

    vsg::ref_ptr<vsg::GlyphMetricsArray> glyphmetrics =
		vsg::GlyphMetricsArray::create( uint32_t( glyphs.size() + 1 ) );
    vsg::ref_ptr<vsg::uintArray> charmap =
				vsg::uintArray::create( cLastChar + 1 );
    for ( auto& entry : *charmap )
	entry = 0;

    vsg::GlyphMetrics nullmetrics{};
    glyphmetrics->set( 0, nullmetrics );

    xpos = cMargin;
    ypos = cMargin;
    rowheight = 0;
    uint32_t destindex = 1;

    for ( const GlyphBitmap* glyph : glyphs )
    {
	const GlyphBitmap& gb = *glyph;
	const bool hasbitmap = gb.hasBitmap();
	const int cellw = hasbitmap ? gb.width + 2 * cMargin : 0;
	const int cellh = hasbitmap ? gb.height + 2 * cMargin : 0;
	if ( hasbitmap )
	{
	    if ( xpos + cellw + cMargin > atlaswidth )
	    {
		xpos = cMargin;
		ypos += rowheight + cMargin;
		rowheight = 0;
	    }

	    for ( int row=0; row<cellh; row++ )
	    {
		const int srcrow = row * cellw;
		const size_t dstrow =
			atlas->index( uint32_t(xpos), uint32_t(ypos + row) );
		for ( int col=0; col<cellw; ++col )
		    atlas->at( dstrow + size_t(col) ) =
			    gb.sdf[srcrow + col];
	    }
	}

	vsg::GlyphMetrics gm{};
	if ( hasbitmap )
	{
	    gm.uvrect.set(
		( float(xpos) - 1.f ) / float( atlaswidth - 1 ),
		float( ypos + cellh ) / float( atlasheight - 1 ),
		float( xpos + cellw ) / float( atlaswidth - 1 ),
		( float(ypos) - 1.f ) / float( atlasheight - 1 ) );
	    gm.width = float( cellw ) / cEmSize;
	    gm.height = float( cellh ) / cEmSize;
	}

	gm.horiBearingX = ( float(gb.originx) - float(cMargin) ) / cEmSize;
	gm.horiBearingY = ( -float(gb.originy) + float(cMargin) ) / cEmSize;
	gm.horiAdvance = gb.horiAdvance;
	gm.vertBearingX = gb.vertBearingX;
	gm.vertBearingY = gb.vertBearingY;
	gm.vertAdvance = gb.vertAdvance;

	glyphmetrics->set( destindex, gm );
	charmap->set( gb.charcode, destindex );
	++destindex;

	if ( hasbitmap )
	{
	    xpos += cellw + cMargin;
	    rowheight = mMAX( rowheight, cellh );
	}
    }

    vsg::ref_ptr<vsg::Font> font = vsg::Font::create();
    font->atlas = atlas;
    font->glyphMetrics = glyphmetrics;
    font->charmap = charmap;
    font->ascender = ascender;
    font->descender = descender;
    font->height = height;
    return font;
}


BufferString systemFamily( const FontData& fd )
{
    BufferString family( fd.family() );
    // Qt's font dialog appends the foundry, as in "Nimbus Sans [urw]".
    // Fontconfig and DirectWrite know the family without that suffix.
    const int bracket = family.indexOf( '[' );
    if ( bracket>1 && family.buf()[bracket-1]==' ' )
	family.replaceAt( bracket-1, nullptr );

#ifdef __win__
    if ( family.isEqual("Helvetica",OD::CaseInsensitive) )
	return BufferString( "Arial" );

    if ( family.isEqual("Courier",OD::CaseInsensitive) )
	return BufferString( "Courier New" );

    if ( family.isEqual("Times",OD::CaseInsensitive) )
	return BufferString( "Times New Roman" );
#endif

    return family;
}


#ifdef __win__

vsg::ref_ptr<vsg::Font> createDirectWriteFont( const FontData& fd )
{
    const BufferString family = systemFamily( fd );
    const bool bold = fd.weight() >= FontData::Bold;

    DirectWriteCOMPtr<IDWriteFactory> factory;
    DirectWriteCOMPtr<IDWriteFontFace> face;
    if ( !openDirectWriteFace(family,bold,fd.isItalic(),factory,face) )
	return {};

    DWRITE_FONT_METRICS fontmetrics{};
    face->GetMetrics( &fontmetrics );
    if ( fontmetrics.designUnitsPerEm == 0 )
	return {};

    const float du_to_px = cEmSize / float( fontmetrics.designUnitsPerEm );

    ManagedObjectSet<GlyphBitmap> glyphs;

    for ( uint32_t ch=cFirstChar; ch<=cLastChar; ++ch )
    {
	const UINT32 codepoint = ch;
	UINT16 glyphindex = 0;
	if ( FAILED(face->GetGlyphIndices(&codepoint,1,&glyphindex))
	     || glyphindex == 0 )
	    continue;

	DWRITE_GLYPH_METRICS gmetrics{};
	if ( FAILED(face->GetDesignGlyphMetrics(&glyphindex,1,
						&gmetrics,FALSE)) )
	    continue;

	FLOAT advance = float( gmetrics.advanceWidth ) * du_to_px;
	DWRITE_GLYPH_OFFSET offset{ 0.f, 0.f };
	DWRITE_GLYPH_RUN run{};
	run.fontFace = face.get();
	run.fontEmSize = cEmSize;
	run.glyphCount = 1;
	run.glyphIndices = &glyphindex;
	run.glyphAdvances = &advance;
	run.glyphOffsets = &offset;
	run.isSideways = FALSE;
	run.bidiLevel = 0;

	DirectWriteCOMPtr<IDWriteGlyphRunAnalysis> analysis;
	if ( FAILED(factory->CreateGlyphRunAnalysis(&run,1.0f,nullptr,
					DWRITE_RENDERING_MODE_ALIASED,
					DWRITE_MEASURING_MODE_NATURAL,
					0.0f,0.0f,&analysis)) )
	    continue;

	RECT bounds{};
	if ( FAILED(analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1,
						    &bounds)) )
	    continue;

	const int width = bounds.right - bounds.left;
	const int height = bounds.bottom - bounds.top;
	const float horiadvance =
		float( gmetrics.advanceWidth ) * du_to_px / cEmSize;
	const float vertbearingx =
		float( gmetrics.leftSideBearing ) * du_to_px / cEmSize;
	const float vertbearingy =
		float( gmetrics.topSideBearing ) * du_to_px / cEmSize;
	const float vertadvance =
		float( gmetrics.advanceHeight ) * du_to_px / cEmSize;
	if ( width <= 0 || height <= 0 )
	{
	    GlyphBitmap* empty = new GlyphBitmap;
	    empty->charcode = ch;
	    empty->horiAdvance = horiadvance;
	    empty->vertBearingX = vertbearingx;
	    empty->vertBearingY = vertbearingy;
	    empty->vertAdvance = vertadvance;
	    glyphs += empty;
	    continue;
	}

	TypeSet<od_uint8> alpha( width*height, 0 );
	if ( FAILED(analysis->CreateAlphaTexture(DWRITE_TEXTURE_ALIASED_1x1,
						 &bounds,alpha.arr(),
						 UINT32(alpha.size()))) )
	    continue;

	GlyphBitmap* gb = new GlyphBitmap;
	gb->charcode = ch;
	gb->width = width;
	gb->height = height;
	gb->originx = bounds.left;
	gb->originy = bounds.top;
	gb->horiAdvance = horiadvance;
	gb->vertBearingX = vertbearingx;
	gb->vertBearingY = vertbearingy;
	gb->vertAdvance = vertadvance;
	makeSdfFromAlpha( alpha, width, height, cMargin, gb->sdf );
	glyphs += gb;
    }

    const float ascender =
		float( fontmetrics.ascent ) * du_to_px / cEmSize;
    const float descender =
		-float( fontmetrics.descent ) * du_to_px / cEmSize;
    const float height = float( fontmetrics.ascent + fontmetrics.descent
				+ fontmetrics.lineGap ) * du_to_px / cEmSize;
    return assembleFont( glyphs, ascender, descender, height );
}

#elif defined(__unix__) && !defined(__mac__)

struct FontFile
{
    BufferString	path_;
    int			index_		= 0;
};


struct FcPatternHolder
{
    FcPattern*		pat_			= nullptr;

			~FcPatternHolder()
			{ if ( pat_ ) FcPatternDestroy( pat_ ); }
};


struct FreeTypeLibrary
{
    FT_Library		lib_			= nullptr;

			~FreeTypeLibrary()
			{ if ( lib_ ) FT_Done_FreeType( lib_ ); }

    bool		init()
			{ return FT_Init_FreeType( &lib_ ) == 0; }
};


/* Fontconfig always returns some face. A requested family is accepted when the
   matched face carries that name, or a name FcConfigSubstitute bound strongly
   (Nimbus Sans L is a strong alias of Nimbus Sans). The generic fallback added
   for an unknown family is only a weak binding, so that face is rejected. */
bool familyMatches( const BufferStringSet& strongnames, FcPattern* match )
{
    FcChar8* got = nullptr;
    for ( int idx=0; FcPatternGetString(match,FC_FAMILY,idx,&got)
							==FcResultMatch; idx++ )
    {
	if ( !got )
	    continue;

	const BufferString gotname( reinterpret_cast<const char*>(got) );
	for ( int sidx=0; sidx<strongnames.size(); sidx++ )
	{
	    if ( gotname.isEqual(strongnames.get(sidx),OD::CaseInsensitive) )
		return true;
	}
    }

    return false;
}


bool findFontFile( const BufferString& family, bool bold, bool italic,
		   FontFile& fontfile )
{
    static std::once_flag inited;
    std::call_once( inited, []() { FcInitLoadConfigAndFonts(); } );

    FcPatternHolder pattern;
    pattern.pat_ = FcPatternCreate();
    if ( !pattern.pat_ )
	return false;

    const FcChar8* wfamily =
		reinterpret_cast<const FcChar8*>( family.buf() );
    FcPatternAddString( pattern.pat_, FC_FAMILY, wfamily );
    FcPatternAddInteger( pattern.pat_, FC_WEIGHT,
			 bold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR );
    FcPatternAddInteger( pattern.pat_, FC_SLANT,
			 italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN );
    FcPatternAddString( pattern.pat_, FC_LANG,
			reinterpret_cast<const FcChar8*>("en") );
    FcConfigSubstitute( nullptr, pattern.pat_, FcMatchPattern );

    BufferStringSet strongnames;
    for ( int idx=0; ; idx++ )
    {
	FcValue value;
	FcValueBinding binding;
	if ( FcPatternGetWithBinding(pattern.pat_,FC_FAMILY,idx,
				     &value,&binding) != FcResultMatch )
	    break;

	if ( binding==FcValueBindingStrong && value.type==FcTypeString &&
	     value.u.s )
	    strongnames.add( reinterpret_cast<const char*>(value.u.s) );
    }

    FcDefaultSubstitute( pattern.pat_ );

    FcResult result;
    FcPatternHolder match;
    match.pat_ = FcFontMatch( nullptr, pattern.pat_, &result );
    if ( !match.pat_ || !familyMatches(strongnames,match.pat_) )
	return false;

    FcChar8* file = nullptr;
    if ( FcPatternGetString(match.pat_,FC_FILE,0,&file) != FcResultMatch
	 || !file )
	return false;

    fontfile.path_ = reinterpret_cast<const char*>( file );
    int index = 0;
    if ( FcPatternGetInteger(match.pat_,FC_INDEX,0,&index) == FcResultMatch )
	fontfile.index_ = index;

    return true;
}


void copyAlpha( const FT_Bitmap& bmp, TypeSet<od_uint8>& alpha )
{
    const int width = int( bmp.width );
    const int height = int( bmp.rows );
    alpha.setSize( width*height, 0 );
    if ( !bmp.buffer || width<=0 || height<=0 )
	return;

    for ( int row=0; row<height; ++row )
    {
	const unsigned char* src = bmp.buffer + row * bmp.pitch;
	od_uint8* dst = alpha.arr() + row * width;
	if ( bmp.pixel_mode == FT_PIXEL_MODE_MONO )
	{
	    for ( int col=0; col<width; ++col )
	    {
		const unsigned char byte = src[col >> 3];
		const unsigned char mask =
				static_cast<unsigned char>( 0x80 >> (col & 7) );
		dst[col] = (byte & mask) ? 255 : 0;
	    }
	}
	else
	{
	    for ( int col=0; col<width; ++col )
		dst[col] = src[col];
	}
    }
}


vsg::ref_ptr<vsg::Font> createFreeTypeFont( const FontData& fd )
{
    const BufferString family = systemFamily( fd );
    const bool bold = fd.weight() >= FontData::Bold;

    FontFile fontfile;
    if ( !findFontFile(family,bold,fd.isItalic(),fontfile) )
	return {};

    FreeTypeLibrary library;
    if ( !library.init() )
	return {};

    FT_Face face = nullptr;
    if ( FT_New_Face(library.lib_,fontfile.path_.buf(),
		     fontfile.index_,&face) )
	return {};

    struct FaceGuard
    {
	FT_Face		face_;
			~FaceGuard()	{ if ( face_ ) FT_Done_Face( face_ ); }
    } guard{ face };

    if ( FT_Set_Pixel_Sizes(face,0,FT_UInt(cEmSize)) )
	return {};

    ManagedObjectSet<GlyphBitmap> glyphs;
    for ( uint32_t ch=cFirstChar; ch<=cLastChar; ++ch )
    {
	const FT_UInt glyphindex = FT_Get_Char_Index( face, ch );
	if ( glyphindex == 0 )
	    continue;

	if ( FT_Load_Glyph(face,glyphindex,FT_LOAD_RENDER|FT_LOAD_MONOCHROME) )
	    continue;

	const FT_GlyphSlot slot = face->glyph;
	GlyphBitmap* gb = new GlyphBitmap;
	gb->charcode = ch;
	gb->width = int( slot->bitmap.width );
	gb->height = int( slot->bitmap.rows );
	gb->originx = slot->bitmap_left;
	gb->originy = -slot->bitmap_top;
	gb->horiAdvance =
		float( slot->metrics.horiAdvance ) / 64.f / cEmSize;
	gb->vertBearingX =
		float( slot->metrics.vertBearingX ) / 64.f / cEmSize;
	gb->vertBearingY =
		float( slot->metrics.vertBearingY ) / 64.f / cEmSize;
	gb->vertAdvance =
		float( slot->metrics.vertAdvance ) / 64.f / cEmSize;
	if ( gb->vertAdvance == 0.f )
	    gb->vertAdvance =
		float( face->size->metrics.height ) / 64.f / cEmSize;

	if ( gb->width>0 && gb->height>0 )
	{
	    TypeSet<od_uint8> alpha;
	    copyAlpha( slot->bitmap, alpha );
	    makeSdfFromAlpha( alpha, gb->width, gb->height, cMargin, gb->sdf );
	}

	glyphs += gb;
    }

    const FT_Size_Metrics& sizemetrics = face->size->metrics;
    const float ascender = float( sizemetrics.ascender ) / 64.f / cEmSize;
    const float descender = float( sizemetrics.descender ) / 64.f / cEmSize;
    const float height = float( sizemetrics.height ) / 64.f / cEmSize;
    return assembleFont( glyphs, ascender, descender, height );
}

#endif

#endif

} // anonymous namespace


namespace
{

class FontAnnouncer : public vsg::Inherit<vsgGeo::Updatable,FontAnnouncer>
{
public:
    void applyUpdates( vsg::Viewer&, vsgGeo::Compiler& ) override
    { FontCreator::instance().announceLoaded(); }
};

} // anonymous namespace


FontCreator& FontCreator::instance()
{
    static FontCreator creator;
    return creator;
}


FontCreator::FontCreator()
    : fontLoaded(this)
{
    mAttachCB( DataObject::commonViewerChanged(),
	       FontCreator::viewerChangedCB );
}


FontCreator::~FontCreator()
{
    detachAllNotifiers();
    clear();
}


void FontCreator::viewerChangedCB( CallBacker* )
{
    if ( DataObject::getCommonViewer() )
	announceLoaded();
    else
	clear();
}


const char* FontCreator::keyFor( const FontData& fd )
{
    mDeclStaticString( key );
    key.set( fd.family() );
    key.add( fd.weight()>=FontData::Bold ? "|bold" : "|normal" );
    key.add( fd.isItalic() ? "|italic" : "|upright" );
    return key.buf();
}


vsg::ref_ptr<vsg::Font> FontCreator::create( const FontData& fd )
{
    vsg::ref_ptr<vsg::Font> font;
#ifdef __win__
    font = createDirectWriteFont( fd );
#elif defined(__unix__) && !defined(__mac__)
    font = createFreeTypeFont( fd );
#else
    (void)fd;
#endif
    if ( !font )
	return {};

    font->createFontImages();
    if ( font->atlasImageInfo && font->atlasImageInfo->sampler )
	font->atlasImageInfo->sampler->maxLod = 0.f;

    return font;
}


vsg::ref_ptr<vsg::Font> FontCreator::get( const FontData& fd ) const
{
    Threads::MutexLocker lock( lock_ );
    const std::map<std::string,vsg::ref_ptr<vsg::Font>>::const_iterator it =
					    cache_.find( keyFor(fd) );
    return it==cache_.end() ? vsg::ref_ptr<vsg::Font>() : it->second;
}


struct FontCreator::LoadJob
{
    FontData		fd_;
    std::string		key_;
};


void FontCreator::loadCB( CallBacker* )
{
    LoadJob* job = nullptr;
    {
	Threads::MutexLocker lock( lock_ );
	if ( jobs_.isEmpty() )
	    return;

	job = jobs_.removeSingle( 0 );
    }

    vsg::ref_ptr<vsg::Font> font = create( job->fd_ );
    {
	    Threads::MutexLocker lck( lock_ );
	loading_.erase( job->key_ );
	if ( font )
	    cache_[job->key_] = font;

	loaded_ += job->fd_;
    }

    delete job;
    postAnnouncement();
}


void FontCreator::request( const FontData& fd )
{
    const std::string key = keyFor( fd );
    Threads::MutexLocker lock( lock_ );
    if ( cache_.count(key) || loading_.count(key) )
	return;

    loading_.insert( key );
    auto* job = new LoadJob;
    job->fd_ = fd;
    job->key_ = key;
    jobs_ += job;
    workers_ += new Threads::Thread( mCB(this,FontCreator,loadCB),
				      "Font loader" );
}


void FontCreator::postAnnouncement()
{
    vsgGeo::Updater* updater = DataObject::getUpdater();
    if ( !updater )
	return;

    vsg::ref_ptr<vsgGeo::Updatable> announcer;
    {
	Threads::MutexLocker lock( lock_ );
	if ( !announcer_ )
	    announcer_ = FontAnnouncer::create();

	announcer = announcer_;
    }

    updater->requestUpdate( announcer );
    DataObject::requestSingleRedraw();
}


void FontCreator::announceLoaded()
{
    TypeSet<FontData> loaded;
    {
	Threads::MutexLocker lock( lock_ );
	loaded.swap( loaded_ );
    }

    for ( const auto& fd : loaded )
	fontLoaded.trigger( fd );
}


void FontCreator::clear()
{
    for ( ;; )
    {
	ObjectSet<Threads::Thread> workers;
	{
	    Threads::MutexLocker lock( lock_ );
	    if ( workers_.isEmpty() )
	    {
		cache_.clear();
		loading_.clear();
		loaded_.erase();
		deepErase( jobs_ );
		announcer_ = vsg::ref_ptr<vsgGeo::Updatable>();
		return;
	    }

	    while ( !workers_.isEmpty() )
		workers += workers_.removeSingle( 0 );
	}

	deepErase( workers );
    }
}

} // namespace dGBVis
