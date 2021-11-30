/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		August 2016
________________________________________________________________________

-*/

#include "uiglinfo.h"

#include "settings.h"
#include "uimsg.h"

#include <osgGeo/GLInfo>


GLInfo::GLInfo()
    : glinfo_( osgGeo::GLInfo::get() )
{
    update();
}


GLInfo::~GLInfo()
{
    delete glinfo_;
}


void GLInfo::update()
{
    isok_ = glinfo_->get();
}


bool GLInfo::isOK() const
{ return isok_; }


bool GLInfo::isPlatformSupported() const
{ return glinfo_ && glinfo_->isPlatformSupported(); }


const char* GLInfo::glVendor() const
{ return glinfo_ ? glinfo_->glVendor() : ""; }


const char* GLInfo::glRenderer() const
{ return glinfo_ ? glinfo_->glRenderer() : ""; }


const char* GLInfo::glVersion() const
{ return glinfo_ ? glinfo_->glVersion() : ""; }


BufferStringSet GLInfo::allInfo() const
{
    BufferStringSet allinfo;

#ifndef  __mac__
    allinfo.add( *glVendor() ? glVendor() : "?" );
    allinfo.add( *glRenderer() ? glRenderer() : "?" );
    allinfo.add( *glVersion() ? glVersion() : "?" );
#endif

    return allinfo;
}


//============================================================================


uiGLInfo* uiGLInfo::theinst_ = 0;

uiGLInfo& uiGLI()
{
    if ( !uiGLInfo::theinst_ )
	uiGLInfo::theinst_ = new uiGLInfo;

    return *uiGLInfo::theinst_;
}


uiString uiGLInfo::getMessage( bool* warning )
{
    uiString msg;
    glinfo_.update();

    if ( !glinfo_.isPlatformSupported() )
    {
	msg = tr("Current platform does not support graphics status messages");
	return msg;
    }

    BufferStringSet allinfo = glinfo_.allInfo();

    msg.append( tr("Scanning your graphics card:") );
    msg.append( "", true );
    msg.append( tr("GL-vendor: "), true );
    msg.append( toUiString(allinfo[0]->buf()) );
    msg.append( tr("GL-renderer: "), true );
    msg.append( toUiString(allinfo[1]->buf()) );
    msg.append( tr("GL-version: "), true );
    msg.append( toUiString(allinfo[2]->buf()) );

    if ( !warning )
	return msg;

    *warning = true;

    if ( !glinfo_.isOK() )
    {
	msg.append( "\n", true );
	msg.append( tr(
	    "Missing all GL info indicates some graphics card problem.") );
    }
    else if ( stringStartsWithCI("intel",allinfo[0]->buf()) )
    {
	msg.append( "\n", true );
	msg.append( tr(
	    "Intel card found. If your computer has multiple graphics cards,\n"
	    "consider switching from the integrated graphics.") );
    }
    else if ( stringStartsWithCI("microsoft",allinfo[0]->buf()) ||
	      stringStartsWithCI("gdi",allinfo[1]->buf()) )
    {
	msg.append( "\n", true );
	msg.append( tr(
	    "No graphics card found or no drivers have been installed.\n"
	    "Please check our system requirements.") );
    }
    else if ( *allinfo[2] == "?" )
    {
	msg.append( "\n", true );
	msg.append( tr(
	    "Missing GL-version indicates a graphics card driver problem.") );
    }
    else
	*warning = false;

    return msg;
}


void uiGLInfo::showMessage( uiString msg, bool warn,
			    const char* dontshowagainkey, bool onlyonce )
{
    BufferStringSet allinfo = glinfo_.allInfo();
    BufferStringSet lastinfo;

    if ( dontshowagainkey )
    {
	Settings::common().get( dontshowagainkey, lastinfo );
	if ( allinfo == lastinfo )
	    return;

	// Don't show message at startup for NVidia cards
	if ( stringStartsWithCI("nvidia",allinfo[0]->buf()) )
	    return;
    }

    const uiString& es = uiString::emptyString();
    const bool askdontshowagain = dontshowagainkey && !onlyonce;
    const bool showagain = warn ?
		    !uiMSG().warning( msg, es, es, askdontshowagain ) :
		    !uiMSG().message( msg, es, es, askdontshowagain );

    if ( dontshowagainkey )
    {
	if ( showagain && !onlyonce )
	{
	    if ( lastinfo.isEmpty() )
		return;

	    allinfo.setEmpty();
	}

	Settings::common().set( dontshowagainkey, allinfo );
	Settings::common().write();
    }
}


void uiGLInfo::createAndShowMessage( bool addwarnings,
				     const char* dontshowagainkey )
{
    bool warning = false;
    const uiString msg = getMessage( addwarnings ? &warning : 0 );
    if ( !msg.isEmpty() && glinfo_.isPlatformSupported() )
	showMessage( msg, warning, dontshowagainkey, addwarnings && !warning );
}
