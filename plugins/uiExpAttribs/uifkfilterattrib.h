#ifndef uifkfilterattrib_h
#define uifkfilterattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2013
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiexpattribsmod.h"
#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;


/*! \brief FK Filter Attribute description editor */

mClass(uiExpAttribs) uiFKFilterAttrib : public uiAttrDescEd
{
public:

			uiFKFilterAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		minmaxfld_;

    void		panelbutCB(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

			mDeclReqAttribUIFns
};

#endif
