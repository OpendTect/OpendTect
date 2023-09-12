/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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


uiGLInfo::uiGLInfo()
{}


uiGLInfo::~uiGLInfo()
{}


uiString uiGLInfo::getMessage( bool* warning )
{
    BufferString msg( "<html>" );
    glinfo_.update();

    const char* url = "https://doc.opendtect.org/7.0.0/doc/admindoc/"
		      "Default.htm#system_requirements.htm";
    BufferString sysreq( "<br><br>Click <a href=\"" );
    sysreq.add( url ).add( "\">" ).add( "here</a>" )
	    .add( " for OpendTect's System Requirements." );

    if ( !glinfo_.isPlatformSupported() )
    {
	msg.add("Current platform does not support graphics status messages");
	msg.add( sysreq );
	msg.add( "<br></html>" );
	return toUiString(msg);
    }

    BufferStringSet allinfo = glinfo_.allInfo();

    msg.add( "<h2>Graphics Card Information</h2><br>" )
       .add( "GL-vendor: " ).add( allinfo[0]->buf() ).add( "<br>" )
       .add( "GL-renderer: " ).add( allinfo[1]->buf() ).add( "<br>" )
       .add( "GL-version: " ).add( allinfo[2]->buf() ).add( "<br>" );

    if ( !warning )
    {
	msg.add( sysreq ).add( "</html>" );
	return toUiString(msg);
    }

    *warning = true;

    msg.add( "<br>" );
    if ( !glinfo_.isOK() )
    {
	msg.add( "Missing all GL info indicates some graphics card problem." );
    }
    else if ( stringStartsWithCI("intel",allinfo[0]->buf()) )
    {
	msg.add(
	    "Intel card found. If your computer has multiple graphics cards,"
	    "<br>consider switching from the integrated graphics." );
    }
    else if ( stringStartsWithCI("ati",allinfo[0]->buf()) ||
	      stringStartsWithCI("amd",allinfo[0]->buf()) )
    {
	msg.add(
	    "AMD card found. Video cards by AMD are not supported.<br>"
	    "Your card may work, but OpendTect will likely experience 3D<br>"
	    "visualization issues. If your computer also has an NVIDIA<br>"
	    "card, make sure OpendTect will use this NVIDIA card." );
    }
    else if ( stringStartsWithCI("microsoft",allinfo[0]->buf()) ||
	      stringStartsWithCI("gdi",allinfo[1]->buf()) )
    {
	msg.add( "No graphics card found or no drivers have been installed." );
    }
    else if ( *allinfo[2] == "?" )
    {
	msg.add("Missing GL-version indicates a graphics card driver problem.");
    }
    else
	*warning = false;

    msg.add( sysreq ).add( "</html>" );
    return toUiString( msg );
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
