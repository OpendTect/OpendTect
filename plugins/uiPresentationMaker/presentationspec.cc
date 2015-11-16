/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: $";


#include "presentationspec.h"

#include "settings.h"
#include "uimain.h"
#include "uipixmap.h"


// SlideLayout

static const char* sWidthStr()	{ return "width"; }
static const char* sHeightStr()	{ return "height"; }
static const char* sLeftStr()	{ return "left"; }
static const char* sRightStr()	{ return "right"; }
static const char* sTopStr()	{ return "top"; }
static const char* sBottomStr()	{ return "bottom"; }
static const char* sFormatStr()	{ return "format"; }
static const char* sMasterIndexStr()	{ return "master index"; }
static const char* sLayoutIndexStr()	{ return "layout index"; }


SlideLayout::SlideLayout()
{
    masterindex_ = 0;
    layoutindex_ = 1;
    format_ = 0;
    width_ = 10.f; height_ = 7.5f;
    left_ = right_ = top_ = bottom_ = 0.f;
    readFromSettings();
}


void SlideLayout::readFromSettings()
{
    Settings& setts = Settings::fetch( "presentation" );
    setts.get( sMasterIndexStr(), masterindex_ );
    setts.get( sLayoutIndexStr(), layoutindex_ );
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
    Settings& setts = Settings::fetch( "presentation" );
    setts.set( sMasterIndexStr(), masterindex_ );
    setts.set( sLayoutIndexStr(), layoutindex_ );
    setts.set( sFormatStr(), format_ );
    setts.set( sWidthStr(), width_ );
    setts.set( sHeightStr(), height_ );
    setts.set( sLeftStr(), left_ );
    setts.set( sRightStr(), right_ );
    setts.set( sTopStr(), top_ );
    setts.set( sBottomStr(), bottom_ );
    setts.write();
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


void SlideContent::getPythonScript( const SlideLayout& layout,
				    BufferString& script ) const
{
    script.add( "slide_master_index=" ).add( layout.masterindex_ ).add( "\n" )
	  .add( "slide_master = prs.slide_masters[slide_master_index]\n" )
	  .add( "slide_layout = slide_master.slide_layouts[" )
	  .add( layout.layoutindex_ ).add( "]\n" )
	  .add( "slide = prs.slides.add_slide(slide_layout)\n" )
	  .add( "title = slide.shapes.title\n" )
	  .add( "title.text = \"" ).add( title_.buf() ).add( "\"\n" );

    SlideContent& myself = const_cast<SlideContent&>(*this);
    if ( !myself.setImageSizePos(layout) )
	return;

    script.add( "left = Inches(" ).add( imagepos_.x ).add( ")\n" )
	  .add( "top = Inches(" ).add( imagepos_.y ).add( ")\n" )
	  .add( "width = Inches(" ).add( imagesz_.width() ).add( ")\n" )
	  .add( "height = Inches(" ).add( imagesz_.height() ).add( ")\n" )
	  .add( "pic = slide.shapes.add_picture('" )
	  .add( imagefnm_.buf() ).add( "'," )
	  .add( "left, top, width, height )\n\n" );
}


PresentationSpec::PresentationSpec()
{
}


PresentationSpec::~PresentationSpec()
{}

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

void PresentationSpec::setOutputFilename( const char* fnm )
{ outputfilename_ = fnm; }

void PresentationSpec::setMasterFilename( const char* fnm )
{ masterfilename_ = fnm; }


static void init( BufferString& script, const char* fnm )
{
    script.add( "from pptx import Presentation\n" )
	  .add( "from pptx.util import Inches, Cm\n\n" );

    script.add( "prs = Presentation(" );
    const FixedString masterfnm = fnm;
    if ( !masterfnm.isEmpty() )
	script.add( "'" ).add( fnm ).add( "'" );

    script.add( ")\n\n" );
}


static void addTitleSlide( BufferString& script, const char* title )
{
    script.add( "title_master = prs.slide_masters[title_master_index]\n" )
	  .add( "title_layout = "
		"title_master.slide_layouts[title_layout_index]\n" )
	  .add( "slide = prs.slides.add_slide(title_layout)\n" )
	  .add( "title = slide.shapes.title\n" )
	  .add( "title.text=\"" ).add( title ).add( "\"\n\n" );
}


void PresentationSpec::getPythonScript( BufferString& script ) const
{
    script.setEmpty();
    init( script, masterfilename_ );
    script.add( "title_master_index=" ).add( titlemasterindex_ ).add( "\n" );
    script.add( "title_layout_index=" ).add( titlelayoutindex_ ).add( "\n" );
    addTitleSlide( script, title_.buf() );

    for ( int idx=0; idx<slides_.size(); idx++ )
	slides_[idx]->getPythonScript( slidelayout_, script );

    script.add( "prs.save('" ).add( outputfilename_ ).add( "')\n" );
}

