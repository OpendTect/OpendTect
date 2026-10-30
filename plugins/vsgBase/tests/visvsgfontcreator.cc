/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "visvsgdata.h"
#include "visvsgfontcreator.h"
#include "thread.h"
#include "timefun.h"
#include "vsggeotext.h"
#include "vsgtesthelper.h"

namespace
{

class LoadedCounter : public CallBacker
{
public:
			LoadedCounter( dGBVis::FontCreator& creator )
			{
			    mAttachCB( creator.fontLoaded,
				       LoadedCounter::loadedCB );
			}

			~LoadedCounter()	{ detachAllNotifiers(); }

    void		loadedCB( CallBacker* cb )
			{
			    mCBCapsuleUnpack(const FontData&,fd,cb);
			    lastloaded_ = fd;
			    nrloaded_++;
			    lastthread_ = Threads::currentThread();
			}

    int			nrloaded_ = 0;
    FontData		lastloaded_;
    Threads::ThreadID	lastthread_ = nullptr;
};


struct CommonViewerGuard
{
    ~CommonViewerGuard()
    {
	dGBVis::DataObject::setCommonViewer( nullptr );
	dGBVis::FontCreator::instance().clear();
    }
};


bool waitForLoaded( vsgTest::OffscreenViewer& ov, const LoadedCounter& counter,
		    int nrexpected )
{
    for ( int idx=0; idx<1500 && counter.nrloaded_<nrexpected; idx++ )
    {
	ov.frame( 1 );
	Threads::sleep( 0.02 );
    }

    return counter.nrloaded_ >= nrexpected;
}

} // namespace


static bool testCreate()
{
    const FontData normal( 24, "Arial", FontData::Normal, false );
    Time::Counter counter;
    counter.start();
    vsg::ref_ptr<vsg::Font> font = dGBVis::FontCreator::create( normal );
    od_cout() << "Making the font took " << counter.elapsed() << " ms"
	      << od_endl;
    mRunStandardTest( font.valid(), "Font creation: Font is made" );
    mRunStandardTest( font->atlas && font->glyphMetrics && font->charmap,
		      "Font creation: Font has an atlas, metrics and "
		      "a character map" );
    mRunStandardTest( font->atlasImageInfo.valid(),
		      "Font creation: Font images are created" );

    const uint32_t idxa = font->glyphIndexForCharcode( U'A' );
    const uint32_t idxg = font->glyphIndexForCharcode( U'g' );
    mRunStandardTest( idxa>0 && idxg>0 && idxa!=idxg,
		      "Font creation: Letters have their own glyphs" );

    const FontData bold( 24, "Arial", FontData::Bold, false );
    vsg::ref_ptr<vsg::Font> boldfont = dGBVis::FontCreator::create( bold );
    mRunStandardTest( boldfont.valid(), "Font creation: Bold font is made" );
    const float normaladv =
	font->glyphMetrics->at( idxa ).horiAdvance;
    const float boldadv =
	boldfont->glyphMetrics->at( boldfont->glyphIndexForCharcode(U'A') )
								.horiAdvance;
    BufferString msg( "Advance of A, normal: ", normaladv );
    msg.add( " bold: " ).add( boldadv );
    mRunStandardTestWithError( normaladv!=boldadv,
			       "Font creation: Bold is really bold", msg );

    const FontData unknown( 24, "NoSuchFontFamilyXyzzy", FontData::Normal,
			    false );
    mRunStandardTest( !dGBVis::FontCreator::create(unknown).valid(),
		      "Font creation: Unknown family gives no font, "
		      "and no crash" );
    return true;
}


static const vsg::GlyphMetrics& spaceMetrics( const vsg::Font& font )
{
    return font.glyphMetrics->at( font.glyphIndexForCharcode(U' ') );
}


static bool testDecorations()
{
    const FontData plain( 24, "Arial", FontData::Normal, false );
    vsg::ref_ptr<vsg::Font> plainfont = dGBVis::FontCreator::create( plain );
    mRunStandardTest( plainfont.valid(), "Decorations: Plain font is made" );

    FontData underlined( plain );
    underlined.setUnderline( true );
    vsg::ref_ptr<vsg::Font> underfont =
				dGBVis::FontCreator::create( underlined );
    mRunStandardTest( underfont.valid(),
		      "Decorations: Underlined font is made" );

    FontData struck( plain );
    struck.setStrikeOut( true );
    vsg::ref_ptr<vsg::Font> strikefont = dGBVis::FontCreator::create( struck );
    mRunStandardTest( strikefont.valid(),
		      "Decorations: Strikeout font is made" );

    const vsg::GlyphMetrics& plainspace = spaceMetrics( *plainfont );
    const vsg::GlyphMetrics& underspace = spaceMetrics( *underfont );
    const vsg::GlyphMetrics& strikespace = spaceMetrics( *strikefont );
    mRunStandardTest( plainspace.width<=0.f,
		      "Decorations: A space has no bitmap without them" );
    mRunStandardTest( underspace.width>0.f,
		      "Decorations: Underline draws a bar under a space" );
    mRunStandardTest( strikespace.width>0.f,
		      "Decorations: Strikeout draws a bar through a space" );
    mRunStandardTest( strikespace.horiBearingY > underspace.horiBearingY,
		      "Decorations: The strikeout bar is above the underline" );
    mRunStandardTest( underspace.horiAdvance == plainspace.horiAdvance,
		      "Decorations: The advance is not changed" );

    return true;
}


static bool testWeights()
{
    const FontData normal( 24, "Arial", FontData::Normal, false );
    const FontData black( 24, "Arial", FontData::Black, false );
    vsg::ref_ptr<vsg::Font> normalfont = dGBVis::FontCreator::create( normal );
    vsg::ref_ptr<vsg::Font> blackfont = dGBVis::FontCreator::create( black );
    mRunStandardTest( normalfont.valid() && blackfont.valid(),
		      "Weights: Normal and Black fonts are made" );

    const float normaladv = normalfont->glyphMetrics->at(
			normalfont->glyphIndexForCharcode(U'A') ).horiAdvance;
    const float blackadv = blackfont->glyphMetrics->at(
			blackfont->glyphIndexForCharcode(U'A') ).horiAdvance;
    BufferString msg( "Advance of A, normal: ", normaladv );
    msg.add( " black: " ).add( blackadv );
    mRunStandardTestWithError( normaladv!=blackadv,
			       "Weights: Black differs from Normal", msg );

    FontData::Weight weights[] = { FontData::Light, FontData::Normal,
				   FontData::DemiBold, FontData::Bold,
				   FontData::Black };
    for ( const FontData::Weight weight : weights )
    {
	const FontData fd( 24, "Arial", weight, false );
	mRunStandardTest( dGBVis::FontCreator::create(fd).valid(),
			  "Weights: A font is made for every weight" );
    }

    return true;
}


static bool testUniversalFamilies()
{
    for ( const char* const* family=FontData::universalFamilies(); *family;
	  family++ )
    {
	const FontData fd( 12, *family, FontData::Normal, false );
	mRunStandardTestWithError( dGBVis::FontCreator::create(fd).valid(),
				   "Font families: A universal family of "
				   "FontData is found",
				   BufferString("Family: ",*family) );
    }

    mRunStandardTestWithError( dGBVis::FontCreator::create(FontData()).valid(),
			       "Font families: The default FontData "
			       "gives a font",
			       BufferString("Default family: ",
					    FontData::defaultFamily()) );

#if defined(__unix__) && !defined(__mac__)
    const FontData nimbus( 12, "Nimbus Sans L", FontData::Normal, false );
    mRunStandardTest( dGBVis::FontCreator::create(nimbus).valid(),
		      "Font families: The Linux default 3D font is found" );

    const FontData dialogname( 18, "Nimbus Sans [urw]", FontData::Normal,
			       false );
    mRunStandardTest( dGBVis::FontCreator::create(dialogname).valid(),
		      "Font families: A font-dialog family with a foundry "
		      "is found" );
#endif
    return true;
}


static bool testRequestAndAnnounce()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Font request: Off-screen viewer",
			       ov.errMsg().c_str() );

    CommonViewerGuard guard;
    dGBVis::DataObject::setCommonViewer( &ov.viewer() );
    mRunStandardTest( ov.compile(), "Font request: Viewer compiled" );

    dGBVis::FontCreator& creator = dGBVis::FontCreator::instance();
    creator.clear();
    LoadedCounter counter( creator );

    const FontData normal( 24, "Arial", FontData::Normal, false );
    creator.request( normal );
    creator.request( normal );
    mRunStandardTest( waitForLoaded(ov,counter,1),
		      "Font request: Font is announced" );
    mRunStandardTest( counter.lastthread_==Threads::currentThread(),
		      "Font request: Announced on the thread that runs "
		      "the Updater" );
    BufferString gotstr, wantedstr;
    counter.lastloaded_.putTo( gotstr );
    normal.putTo( wantedstr );
    BufferString fdmsg( "Got: ", gotstr );
    fdmsg.add( ", wanted: " ).add( wantedstr );
    mRunStandardTestWithError( gotstr==wantedstr,
			       "Font request: Announcement carries the "
			       "requested FontData", fdmsg );
    mRunStandardTest( creator.get(normal).valid(),
		      "Font request: Font is cached" );

    ov.frame( 5 );
    mRunStandardTestWithError( counter.nrloaded_==1,
			       "Font request: Requesting twice loads once",
			       BufferString("Announced: ",counter.nrloaded_) );

    const FontData larger( 48, "Arial", FontData::Normal, false );
    mRunStandardTest( creator.get(larger)==creator.get(normal),
		      "Font request: Point size does not make another font" );
    creator.request( larger );
    ov.frame( 5 );
    mRunStandardTest( counter.nrloaded_==1,
		      "Font request: A cached font is not announced again" );

    const FontData unknown( 24, "NoSuchFontFamilyXyzzy", FontData::Normal,
			    false );
    creator.request( unknown );
    mRunStandardTest( waitForLoaded(ov,counter,2),
		      "Font request: A failed font is announced too" );
    mRunStandardTest( !creator.get(unknown).valid(),
		      "Font request: A failed font is not cached" );

    mRunStandardTest( ov.nrValidationMessages()==0,
		      "Font request: No validation messages" );
    return true;
}


static bool testDrawn()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Created font: Off-screen viewer",
			       ov.errMsg().c_str() );

    CommonViewerGuard guard;
    dGBVis::DataObject::setCommonViewer( &ov.viewer() );
    vsgGeo::Updater* updater = dGBVis::DataObject::getUpdater();
    mRunStandardTest( updater!=nullptr, "Created font: Common updater exists" );

    dGBVis::FontCreator& creator = dGBVis::FontCreator::instance();
    creator.clear();
    LoadedCounter counter( creator );

    const FontData normal( 24, "Arial", FontData::Normal, false );
    creator.request( normal );
    mRunStandardTest( ov.compile(), "Created font: Viewer compiled" );
    mRunStandardTest( waitForLoaded(ov,counter,1),
		      "Created font: Font is loaded" );

    vsg::ref_ptr<vsgGeo::Text> text = vsgGeo::Text::create();
    text->setUpdater( updater );
    text->setFont( creator.get(normal) );
    text->setText( U"Hi" );
    text->setCharacterHeight( 0.5f );
    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::XZ );
    text->setAlignment( vsgGeo::Text::HAlign::Center,
			vsgGeo::Text::VAlign::Center );
    ov.scene().addChild( text );
    ov.frame( 3 );

    const vsgTest::Bounds bnds = vsgTest::brightBounds( ov );
    BufferString msg( "Bright pixels: ", bnds.nrpix_ );
    msg.add( ", centre x: " ).add( bnds.cx() ).add( " y: " ).add( bnds.cy() );
    mRunStandardTestWithError( bnds.nrpix_>50,
			       "Created font: A label with the made font "
			       "is drawn", msg );
    mRunStandardTestWithError( abs(bnds.cx()-128)<=12 && abs(bnds.cy()-128)<=12,
			       "Created font: The label is at the anchor",
			       msg );

    mRunStandardTest( updater->compiler().nrFailedCompiles()==0,
		      "Created font: No failed compiles" );
    msg.set( "Messages: " ).add( ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Created font: No validation messages", msg );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testCreate() ||
	 !testDecorations() ||
	 !testWeights() ||
	 !testUniversalFamilies() ||
	 !testRequestAndAnnounce() ||
	 !testDrawn() )
	return 1;

    return 0;
}
