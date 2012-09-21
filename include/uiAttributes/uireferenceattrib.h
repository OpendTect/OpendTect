#ifndef uireferenceattrib_h
#define uireferenceattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          July 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiImagAttrSel;
class uiGenInput;

/*! \brief Reference Attribute description editor */

mClass(uiAttributes) uiReferenceAttrib : public uiAttrDescEd
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

