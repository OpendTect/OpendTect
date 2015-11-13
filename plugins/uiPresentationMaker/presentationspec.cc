/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: $";


#include "presentationspec.h"
#include "slidespec.h"


PresSlideSpec::PresSlideSpec()
    : index_(-1)
    , layoutindex_(1)
    , imagesz_(0,0)
    , imagepos_(0,0)
{
    imagefnm_.setEmpty();
}


PresSlideSpec::~PresSlideSpec()
{}


void PresSlideSpec::getPythonScript( BufferString& script ) const
{
    script.add( "slide_layout = prs.slide_layouts[" )
	  .add( layoutindex_ ).add( "]\n" )
	  .add( "slide = prs.slides.add_slide(slide_layout)\n" );

    if ( !imagefnm_.isEmpty() )
	script.add( "pic = slide.shapes.add_picture('" )
	      .add( imagefnm_.buf() ).add( "',0,0)\n" );
}


PresentationSpec::PresentationSpec()
{}


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


void PresentationSpec::addSlide( PresSlideSpec& ss )
{ slides_ += &ss; }

void PresentationSpec::insertSlide( int idx, PresSlideSpec& ss )
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

    script.add( ")\n" );
}


static void addTitleSlide( BufferString& script, const char* title )
{
    script.add( "title_slide_layout = prs.slide_layouts[0]\n" )
	  .add( "slide = prs.slides.add_slide(title_slide_layout)\n" )
	  .add( "title = slide.shapes.title\n" )
	  .add( "title.text=\"" ).add( title ).add( "\"\n" );
}


void PresentationSpec::getPythonScript( BufferString& script ) const
{
    script.setEmpty();
    init( script, masterfilename_ );
    addTitleSlide( script, title_.buf() );

    for ( int idx=0; idx<slides_.size(); idx++ )
	slides_[idx]->getPythonScript( script );

    script.add( "prs.save('" ).add( outputfilename_ ).add( "')\n" );
}

