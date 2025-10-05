/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiglinfo.h"

#include "settings.h"
#include "uimsg.h"
#include "visosg.h"

#include <osgGeo/GLInfo>


GLInfo::GLInfo()
    : glinfo_( osgGeo::GLInfo::get() )
{
    visBase::refOsgPtr( glinfo_ );
    update();
}


GLInfo::~GLInfo()
{
    visBase::unRefOsgPtr( glinfo_ );
}


void GLInfo::update()
{
    isok_ = glinfo_->get();
}


bool GLInfo::isOK() const
{
    return isok_;
}


bool GLInfo::isPlatformSupported() const
{
    return glinfo_ && glinfo_->isPlatformSupported();
}


const char* GLInfo::glVendor() const
{
    return glinfo_ ? glinfo_->glVendor() : "";
}


const char* GLInfo::glRenderer() const
{
    return glinfo_ ? glinfo_->glRenderer() : "";
}


const char* GLInfo::glVersion() const
{
    return glinfo_ ? glinfo_->glVersion() : "";
}


// uiGLInfo

uiGLInfo& uiGLI()
{
    static PtrMan<uiGLInfo> theinst = new uiGLInfo;
    return *theinst.ptr();
}


uiGLInfo::uiGLInfo()
{}


uiGLInfo::~uiGLInfo()
{}


uiRetVal uiGLInfo::getInfo( IOPar& iop, uiRetVal* warnings, bool needupdate )
{
    if ( needupdate )
	glinfo_.update();

    if ( !glinfo_.isPlatformSupported() )
	return tr("Current platform does not support graphics status messages");

    const BufferString glvendor( glinfo_.glVendor() );
    const BufferString glrender( glinfo_.glRenderer() );
    const BufferString glversion( glinfo_.glVersion() );
    iop.set( "GL-vendor", glvendor.buf() );
    iop.set( "GL-renderer", glrender.buf() );
    iop.set( "GL-version", glversion.buf() );
    if ( !warnings )
	return uiRetVal::OK();

    if ( !glinfo_.isOK() )
    {
	*warnings = tr("Missing all GL info indicates some graphics "
		       "card problem.");
    }
    else if ( glvendor.startsWith("intel",OD::CaseInsensitive) )
    {
	warnings->add( tr("Intel card found. If your computer has "
			  "multiple graphics cards,") )
		 .add( tr("consider switching from the integrated graphics") );
    }
    else if ( glvendor.startsWith("amd",OD::CaseInsensitive) ||
	      glvendor.startsWith("ati",OD::CaseInsensitive) )
    {
	warnings->add(
	      tr("AMD card found. Video cards by AMD are not supported.") )
	.add( tr("Your card may work, but OpendTect will likely experience 3D"))
	.add( tr("visualization issues. If your computer also has an NVIDIA") )
	.add( tr("card, make sure OpendTect will use this NVIDIA card.") );
    }
    else if ( glvendor.startsWith("microsoft",OD::CaseInsensitive) ||
	      glrender.startsWith("gdi",OD::CaseInsensitive) )
    {
	warnings->add(
	    tr("No graphics card found or no drivers have been installed.") );
    }
    else if ( glversion == "?" )
    {
	warnings->add(
	   tr("Missing GL-version indicates a graphics card driver problem.") );
    }

    return uiRetVal::OK();
}


uiString uiGLInfo::getMessage( const uiRetVal& uirv, const uiRetVal& warnings )
{
    BufferString msg( "<html>" );

    const char* url = "https://doc.opendtect.org/2025/doc/admindoc/"
		      "Default.htm#system_requirements.htm";
    BufferString sysreq( "<br><br>Click <a href=\"" );
    sysreq.add( url ).add( "\">" ).add( "here</a>" )
	  .add( " for OpendTect's System Requirements." ).add( "</html>" );

    if ( !uirv.isOK() )
    {
	const uiPhraseSet msgs = uirv.messages();
	for ( const auto* phr : msgs )
	{
	    BufferString txt;
	    phr->fillUTF8String( txt );
	    msg.add( txt ).add( "<br>" );
	}

	msg.add( sysreq );
	return toUiString( msg );
    }

    msg.add( "<h2>Graphics Card Information</h2><br>" )
       .add( "GL-vendor: " ).add( glinfo_.glVendor() ).add( "<br>" )
       .add( "GL-renderer: " ).add( glinfo_.glRenderer() ).add( "<br>" )
       .add( "GL-version: " ).add( glinfo_.glVersion() ).add( "<br>" );

    if ( !warnings.isOK() )
    {
	const uiPhraseSet msgs = warnings.messages();
	for ( const auto* phr : msgs )
	{
	    BufferString txt;
	    phr->fillUTF8String( txt );
	    msg.add( txt ).add( "<br>" );
	}
    }

    msg.add( sysreq );

    return toUiString( msg );
}


void uiGLInfo::showMessage( const uiString& msg, bool warn,
			    const char* dontshowagainkey, bool onlyonce )
{
    const BufferString glvendor( glinfo_.glVendor() );
    const BufferString glrender( glinfo_.glRenderer() );
    const BufferString glversion( glinfo_.glVersion() );
    const BufferStringSet allinfo( glvendor, glrender, glversion );
    BufferStringSet lastinfo;
    if ( dontshowagainkey )
    {
	Settings::common().get( dontshowagainkey, lastinfo );
	if ( allinfo == lastinfo )
	    return;

	// Don't show message at startup for NVidia cards
	if ( glvendor.startsWith("nvidia",OD::CaseInsensitive) )
	{
	    if ( !lastinfo.isEmpty() )
	    {
		Settings::common().reRead();
		Settings::common().removeWithKey( dontshowagainkey );
		Settings::common().write( false );
	    }

	    return;
	}
    }

    const uiString& es = uiString::empty();
    const bool askdontshowagain = dontshowagainkey && !onlyonce;
    const bool showagain = warn
			 ? !uiMSG().warning( msg, es, es, askdontshowagain )
			 : !uiMSG().message( msg, es, es, askdontshowagain );
    if ( dontshowagainkey )
    {
	if ( showagain && !onlyonce )
	{
	    if ( lastinfo.isEmpty() )
		return;

	    Settings::common().reRead();
	    Settings::common().removeWithKey( dontshowagainkey );
	    Settings::common().write( false );
	}
	else
	{
	    Settings::common().set( dontshowagainkey, allinfo );
	    Settings::common().write();
	}
    }
}


void uiGLInfo::createAndShowMessage( bool addwarnings, IOPar* graphicspar,
				     const char* dontshowagainkey )
{
    glinfo_.update();
    if ( !glinfo_.isPlatformSupported() )
	return;

    IOPar par;
    uiRetVal warnings;
    const uiRetVal uirv = getInfo( par, addwarnings ? &warnings : nullptr,
				   false );
    const uiString msg = getMessage( uirv, warnings );
    const bool warnonce = addwarnings && warnings.isEmpty();
    showMessage( msg, !warnings.isEmpty(), dontshowagainkey, warnonce );
    if ( graphicspar )
	*graphicspar = par;
}
