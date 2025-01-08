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

    void		createAndShowMessage(bool addwarnings=false,
				    IOPar* graphicspar=nullptr,
				    const char* dontshowagainkey=nullptr);

private:
    uiRetVal		getInfo(IOPar&,uiRetVal* warnings,bool needupdate=true);
    uiString		getMessage(const uiRetVal& error,
				   const uiRetVal& warnings);
    void		showMessage(const uiString& msg,bool warn=false,
				    const char* dontshowagainkey=nullptr,
				    bool onlyonce=false);

    GLInfo		glinfo_;
};

mGlobal(uiOSG) uiGLInfo& uiGLI();
