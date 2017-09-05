#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		August 2016
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

    bool		isOK() const;
    bool		isPlatformSupported() const;

    const char*		glVendor() const;
    const char*		glRenderer() const;
    const char*		glVersion() const;

    BufferStringSet	allInfo() const;

protected:
};


mExpClass(uiOSG) uiGLInfo
{ mODTextTranslationClass(uiGLInfo)
public:
    friend mGlobal(uiOSG) uiGLInfo& uiGLI();

			uiGLInfo()					{}

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

