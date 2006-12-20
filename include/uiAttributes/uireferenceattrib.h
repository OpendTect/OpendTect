#ifndef uireferenceattrib_h
#define uireferenceattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          July 2005
 RCS:           $Id: uireferenceattrib.h,v 1.6 2006-12-20 11:23:00 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiImagAttrSel;
class uiGenInput;

/*! \brief Reference Attribute description editor */

class uiReferenceAttrib : public uiAttrDescEd
{
public:

			uiReferenceAttrib(uiParent*,bool);

protected:

    uiAttrSel*          inpfld;
    uiGenInput*		outpfld3d;
    uiGenInput*		outpfld2d;

    static const char*	outstrs3d[];
    static const char*	outstrs2d[];

    bool		setParameters(const Attrib::Desc&);
    bool                setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getOutput(Attrib::Desc&);
    bool                getInput(Attrib::Desc&);
    bool                getParameters(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
