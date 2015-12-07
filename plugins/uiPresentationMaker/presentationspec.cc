/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: $";


#include "presentationspec.h"

#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "settings.h"
#include "uimain.h"
#include "uipixmap.h"


static const char* sSettingsKey = "presentation";

// SlideLayout

static const char* sWidthStr()	{ return "width"; }
static const char* sHeightStr()	{ return "height"; }
static const char* sLeftStr()	{ return "left"; }
static const char* sRightStr()	{ return "right"; }
static const char* sTopStr()	{ return "top"; }
static const char* sBottomStr()	{ return "bottom"; }
static const char* sFormatStr()	{ return "format"; }


SlideLayout::SlideLayout()
{
    forBlankPresentation();
    readFromSettings();
}


void SlideLayout::readFromSettings()
{
    Settings& setts = Settings::fetch( sSettingsKey );
    setts.get( sFormatStr(), format_ );
    setts.get( sWidthStr(), width_ );
    setts.get( sHeightStr(), height_ );
    setts.get( sLeftStr(), left_ );
    setts.get( sRightStr(), right_ );
    setts.get( sTopStr(), top_ );
    setts.get( sBottomStr(), bottom_ );
}


void SlideLayout::saveToSettings()
{
    Settings& setts = Settings::fetch( sSettingsKey );
    setts.set( sFormatStr(), format_ );
    setts.set( sWidthStr(), width_ );
    setts.set( sHeightStr(), height_ );
    setts.set( sLeftStr(), left_ );
    setts.set( sRightStr(), right_ );
    setts.set( sTopStr(), top_ );
    setts.set( sBottomStr(), bottom_ );
    setts.write();
}


void SlideLayout::forBlankPresentation()
{
    format_ = 0;
    width_ = 10.f; height_ = 7.5f;
    left_ = right_ = 0.5f;
    top_ = 1.7f; bottom_ = 0.7f;
}


float SlideLayout::availableWidth() const
{ return width_ - right_ - left_; }

float SlideLayout::availableHeigth() const
{ return height_ - top_ - bottom_; }



// SlideContent
SlideContent::SlideContent( const char* title, const char* imgfnm )
    : imagesz_(0,0)
    , imagepos_(0,0)
    , title_(title)
    , imagefnm_(imgfnm)
{
}


SlideContent::~SlideContent()
{}


void SlideContent::setTitle( const char* title )
{ title_ = title; }


bool SlideContent::setImageSizePos( const SlideLayout& layout )
{
    if ( imagefnm_.isEmpty() )
	return false;

    uiPixmap pm( imagefnm_ );
    const int widthpix = pm.width();
    const int heightpix = pm.height();
    const int dpi = uiMain::getDPI();
    const float width = (float)widthpix / dpi;
    const float height = (float)heightpix / dpi;

    const float avwidth = layout.availableWidth();
    const float avheight = layout.availableHeigth();

    float factor = avwidth / width;
    if ( factor * height > avheight )
	factor = avheight / height;

    const float newwidth = width * factor;
    const float newheight = height * factor;
    const float w0 = layout.left_ + avwidth/2 - newwidth/2;
    const float h0 = layout.top_ + avheight/2 - newheight/2;

    imagesz_.set( newwidth, newheight );
    imagepos_.setXY( w0, h0 );
    return true;
}


void SlideContent::addBlankSlide( BufferString& script )
{
    script.add(
	"slide = prs.slides.add_slide(slide_layout)\n"
	"title = slide.shapes.title\n" );
    addTitle( script );
    addImage( script );
}


void SlideContent::addAsFirstSlide( BufferString& script )
{
    script.add( "title = first_slide.shapes.title\n" );
    addTitle( script );
    addImage( script );
}


void SlideContent::addWithFirstSlideLayout( BufferString& script )
{
    script.add(
	"slide = prs.slides.add_slide(slide_layout)\n"
	"title = slide.shapes.title\n" );
    addTitle( script );
    addImage( script );
}


void SlideContent::addTitle( BufferString& script )
{
    script.add( "title.text = '" ).add( title_.buf() ).add( "'\n" );
}


void SlideContent::addImage( BufferString& script )
{
    script.add( "left = Inches(" ).add( imagepos_.x ).add( ")\n" )
	  .add( "top = Inches(" ).add( imagepos_.y ).add( ")\n" )
	  .add( "width = Inches(" ).add( imagesz_.width() ).add( ")\n" )
	  .add( "height = Inches(" ).add( imagesz_.height() ).add( ")\n" )
	  .add( "pic = slide.shapes.add_picture('" )
	  .add( imagefnm_.buf() ).add( "'," )
	  .add( "left, top, width, height )\n\n" );
}


// PresentationSpec
PresentationSpec::PresentationSpec()
{
}


PresentationSpec::~PresentationSpec()
{}


static const char* sPyDir()	{ return "python-pptx directory"; }
static const char* sTemplate()	{ return "template pptx"; }

BufferString PresentationSpec::getPyDir()
{
    const BufferString dir =
	FilePath( GetDataDir() ).add("Misc").add("python-pptx").fullPath();
    if ( !File::exists(dir.buf()) )
	File::createDir( dir );

    return dir;
}


BufferString PresentationSpec::getTemplate()
{
    Settings& setts = Settings::fetch( sSettingsKey );
    BufferString fnm;
    setts.get( sTemplate(), fnm );
    return fnm;
}


void PresentationSpec::setTemplate( const char* fnm )
{
    Settings& setts = Settings::fetch( sSettingsKey );
    setts.set( sTemplate(), fnm );
    setts.write();
}


void PresentationSpec::setEmpty()
{
    deepErase( slides_ );
    title_.setEmpty();
    outputfilename_.setEmpty();
    masterfilename_.setEmpty();
}


int PresentationSpec::nrSlides() const
{ return slides_.size(); }


void PresentationSpec::addSlide( SlideContent& ss )
{ slides_ += &ss; }

void PresentationSpec::insertSlide( int idx, SlideContent& ss )
{ slides_.insertAt( &ss, idx ); }

void PresentationSpec::swapSlides( int idx0, int idx1 )
{ slides_.swap( idx0, idx1 ); }

void PresentationSpec::removeSlide( int idx )
{ delete slides_.removeSingle( idx ); }

void PresentationSpec::setTitle( const char* title )
{ title_ = title; }


void PresentationSpec::setSlideTitle( int idx, const char* title )
{
    const FixedString fs = title;
    if ( slides_.validIdx(idx) && !fs.isEmpty() )
	slides_[idx]->setTitle( title );
}

void PresentationSpec::setOutputFilename( const char* fnm )
{ outputfilename_ = fnm; }

void PresentationSpec::setMasterFilename( const char* fnm )
{ masterfilename_ = fnm; }


static void init( BufferString& script, const char* fnm )
{
    script.add(
	"from pptx import Presentation\n"
	"from pptx.util import Inches, Cm\n"
	"import os.path\n\n" );

    script.add( "prs = Presentation(" );
    const FixedString masterfnm = fnm;
    if ( !masterfnm.isEmpty() )
	script.add( "'" ).add( fnm ).add( "'" );

    script.add( ")\n\n" );
}


static void addTitleSlide( BufferString& script, const char* title )
{
    script.add(
	"title_master_index = 0\n"
	"title_layout_index = 0\n"
	"title_master = prs.slide_masters[title_master_index]\n"
	"title_layout = title_master.slide_layouts[title_layout_index]\n"
	"title_slide = prs.slides.add_slide(title_layout)\n"
	"title = title_slide.shapes.title\n" )
    .add( "title.text = '" ).add( title ).add( "'" ).addNewLine(2);
}


static void initTitleSlide( BufferString& script, const char* title )
{
    script.add(
	"title_slide = prs.slides[0]\n"
	"title_slide.shapes.title.text = " );
    script.add( "'" ).add( title ).add( "'" ).addNewLine(2);
}


static void initSlides( BufferString& script, bool isblankpres )
{
    if ( isblankpres )
    {
	script.add(
	    "slide_layout_index = 1\n"
	    "slide_layout = prs.slide_layouts[slide_layout_index]\n"
	    );
    }
    else
    {
	script.add(
	    "first_slide = prs.slides[1]\n"
	    "slide_layout = first_slide.slide_layout\n"
	    "slide = first_slide\n"
	    );
    }

    script.addNewLine();
}


void PresentationSpec::getPythonScript( BufferString& script )
{
    script.setEmpty();
    init( script, masterfilename_ );
    const bool isblankpres = masterfilename_.isEmpty();
    if ( isblankpres )
    {
	slidelayout_.forBlankPresentation();
	addTitleSlide( script, title_.buf() );
    }
    else
    {
	slidelayout_.readFromSettings();
	initTitleSlide( script, title_.buf() );
    }

    if ( !slides_.isEmpty() )
    {
	initSlides( script, isblankpres );
	slides_[0]->setImageSizePos( slidelayout_ );
	if ( isblankpres )
	    slides_[0]->addBlankSlide( script );
	else
	    slides_[0]->addAsFirstSlide( script );

	for ( int idx=1; idx<slides_.size(); idx++ )
	{
	    slides_[idx]->setImageSizePos( slidelayout_ );
	    if ( isblankpres )
		slides_[idx]->addBlankSlide( script );
	    else
		slides_[idx]->addWithFirstSlideLayout( script );
	}
    }

    script.add( "outputname = os.path.normpath('" );
    BufferString outputfnm = outputfilename_;
#ifdef __win__
    outputfnm.replace( "\\", "/" );
#endif
    script.add( outputfnm );
    script.add( "')\n" );
    script.add( "prs.save(outputname)\n" );
}
