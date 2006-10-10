#ifndef uiinstantattrib_h
#define uiinstantattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uiinstantattrib.h,v 1.4 2006-10-10 17:46:05 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiImagAttrSel;
class uiGenInput;

/*! \brief Instantaneous Attribute description editor */

class uiInstantaneousAttrib : public uiAttrDescEd
{
public:

			uiInstantaneousAttrib(uiParent*);

protected:

    uiImagAttrSel*	inpfld;
    uiGenInput*		outpfld;

    static const char*	outstrs[];

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
