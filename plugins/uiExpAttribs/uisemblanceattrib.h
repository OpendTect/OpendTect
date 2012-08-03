#ifndef uisemblanceattrib_h
#define uisemblanceattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Jan 2008
 RCS:           $Id: uisemblanceattrib.h,v 1.4 2012-08-03 13:01:33 cvskris Exp $
________________________________________________________________________

-*/

#include "uiexpattribsmod.h"
#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;
class uiSteeringSel;


/*! \brief Semblance Attribute description editor */

mClass(uiExpAttribs) uiSemblanceAttrib : public uiAttrDescEd
{
public:

			uiSemblanceAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld;
    uiSteeringSel*	steerfld;
    uiGenInput*		gatefld;
    uiGenInput*		extfld;
    uiStepOutSel*	pos0fld;
    uiStepOutSel*	pos1fld;
    uiStepOutSel*	stepoutfld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    void		extSel(CallBacker*);

    			mDeclReqAttribUIFns
};


#endif

