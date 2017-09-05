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
{
}


GLInfo::~GLInfo()
{
}


bool GLInfo::isOK() const
{
    osg::ref_ptr<osgGeo::GLInfo> glinfo = osgGeo::GLInfo::get();
    return glinfo.valid() ? glinfo->isOK() : false;
}


bool GLInfo::isPlatformSupported() const
{ return true; }


const char* GLInfo::glVendor() const
{
    osg::ref_ptr<osgGeo::GLInfo> glinfo = osgGeo::GLInfo::get();
    return glinfo->glVendor();
}


const char* GLInfo::glRenderer() const
{
    osg::ref_ptr<osgGeo::GLInfo> glinfo = osgGeo::GLInfo::get();
    return glinfo->glRenderer();
}


const char* GLInfo::glVersion() const
{
    osg::ref_ptr<osgGeo::GLInfo> glinfo = osgGeo::GLInfo::get();
    return glinfo->glVersion();
}


#define mGLInfoStr(str,fn) \
    BufferString(str,toString(glinfo->fn()))

BufferStringSet GLInfo::allInfo() const
{
    osg::ref_ptr<osgGeo::GLInfo> glinfo = osgGeo::GLInfo::get();
    BufferStringSet allinfo;
    allinfo.add( *glVendor() ? glVendor() : "?" )
	.add( *glRenderer() ? glRenderer() : "?" )
	.add( *glVersion() ? glVersion() : "?" )
	.add( toString(glinfo->isVertexProgramSupported()) )
	.add( toString(glinfo->isShaderProgramSupported()) )
	.add( toString(glinfo->isGeometryShader4Supported()) );

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
    if ( !glinfo_.isOK() )
    {
	msg = tr("Cannot access graphics status messages");
	return msg;
    }

    if ( !glinfo_.isPlatformSupported() )
    {
	msg = tr("Current platform does not support graphics status messages");
	return msg;
    }

#define mAddStr(txt,idx) \
    msg.append( txt, true ).append( toUiString(allinfo[idx]->buf()) );

    BufferStringSet allinfo = glinfo_.allInfo();
    msg.setEmpty();
    msg.append( tr("Scanning your graphics card:") );
    msg.append( "", true );
    mAddStr(tr("GL-vendor: "), 0 );
    mAddStr(tr("GL-renderer: "), 1 );
    mAddStr(tr("GL-version: "), 2 );
    msg.append( "", true );
    mAddStr(tr("Vertex shader support: "), 3 );
    mAddStr(tr("Fragment shader support: "), 3 );
    mAddStr(tr("Geometry shader4 support: "), 5 );

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
	msg.append( tr("Intel card found. Consider switching if your computer "
		       "has more than 1 graphics card."));
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
    if ( !msg.isEmpty() )
	showMessage( msg, warning, dontshowagainkey, addwarnings && !warning );
}
