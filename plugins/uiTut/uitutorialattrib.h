#ifndef uitutorialattrib_h
#define uitutorialattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          May 2007
 RCS:           $Id: uitutorialattrib.h,v 1.5 2012-08-07 04:23:04 cvsmahant Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiSteeringSel;
class uiStepOutSel;

//Modified

class uiLabeledSpinBox;
class uiPushButton;

//Modified


class uiTutorialAttrib : public uiAttrDescEd
{
public:

			uiTutorialAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		actionfld_;
    uiGenInput*         actionfld1_;//Modified
    uiGenInput*         scalespeedfld_;//Modified
    uiGenInput*		factorfld_;
    uiGenInput*		shiftfld_;
    uiGenInput*         indexfld_;//Modified
    uiGenInput*		smoothstrengthfld_;
    uiGenInput*         smoothdirfld_;
    uiSteeringSel*      steerfld_;
    uiStepOutSel*       stepoutfld_;


    void		actionSel(CallBacker*);
    void                steerTypeSel(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
