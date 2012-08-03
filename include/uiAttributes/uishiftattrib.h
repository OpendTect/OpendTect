#ifndef uishiftattrib_h
#define uishiftattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uishiftattrib.h,v 1.11 2012-08-03 13:00:49 cvskris Exp $
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;
class uiSteeringSel;

/*! \brief Shift Attribute description editor */

mClass(uiAttributes) uiShiftAttrib : public uiAttrDescEd
{
public:

			uiShiftAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>& params) const;

protected:

    uiAttrSel*          inpfld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput*		timefld_;
    uiGenInput*		dosteerfld_;
    uiSteeringSel*	steerfld_;

    void		steerSel(CallBacker*);
    void                steerTypeSel(CallBacker*);
    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif

