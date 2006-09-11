#ifndef uiinstantattrib_h
#define uiinstantattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uiinstantattrib.h,v 1.3 2006-09-11 07:04:12 cvsnanne Exp $
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
    static void		initClass();
			uiInstantaneousAttrib(uiParent*);

    const char*		getAttribName() const;

protected:
    static uiAttrDescEd* createInstance(uiParent*);

    uiImagAttrSel*	inpfld;
    uiGenInput*		outpfld;

    static const char*	outstrs[];

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);
};

#endif
