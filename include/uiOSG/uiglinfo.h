#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiosgmod.h"
#include "uistring.h"
#include "bufstringset.h"

namespace osgGeo	{ class GLInfo; }


mExpClass(uiOSG) GLInfo
{
public:
			GLInfo();
			~GLInfo();

    void		update();

    bool		isOK() const;
    bool		isPlatformSupported() const;

    const char*		glVendor() const;
    const char*		glRenderer() const;
    const char*		glVersion() const;

    BufferStringSet	allInfo() const;

protected:

    bool		isok_;
    osgGeo::GLInfo*	glinfo_;
};


mExpClass(uiOSG) uiGLInfo
{ mODTextTranslationClass(uiGLInfo)
public:
    friend mGlobal(uiOSG) uiGLInfo& uiGLI();

			uiGLInfo();
			~uiGLInfo();

    uiString		getMessage(bool* warning=0);
    void		showMessage(uiString msg,bool warn=false,
				    const char* dontshowagainkey=0,
				    bool onlyonce=false);
    void		createAndShowMessage(bool addwarnings=false,
				    const char* dontshowagainkey=0);
protected:
    GLInfo		glinfo_;
    static uiGLInfo*	theinst_;
};

mGlobal(uiOSG) uiGLInfo& uiGLI();
