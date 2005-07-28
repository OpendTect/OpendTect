#ifndef uireferenceattrib_h
#define uireferenceattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          July 2005
 RCS:           $Id: uireferenceattrib.h,v 1.1 2005-07-28 10:53:49 cvshelene Exp $
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

			uiReferenceAttrib(uiParent*);

protected:

    uiGenInput*		outpfld;

    static const char*	outstrs[];

    bool		setParameters(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getOutput(Attrib::Desc&);
};

#endif
