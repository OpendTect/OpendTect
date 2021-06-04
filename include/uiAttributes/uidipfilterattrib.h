#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;

/*! \brief DipFilter Attribute description editor */

mExpClass(uiAttributes) uiDipFilterAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiDipFilterAttrib);
public:

			uiDipFilterAttrib(uiParent*,bool);

    virtual void	getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiLabeledSpinBox*	szfld_;
    uiGenInput*		fltrtpfld_;
    uiGenInput*		velfld_;
    uiGenInput*		azifld_;
    uiGenInput*		aziintfld_;
    uiGenInput*		taperfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    void		panelbutCB(CallBacker*);
    void		fkWinCloseCB(CallBacker*);
    void		filtSel(CallBacker*);
    void		aziSel(CallBacker*);

			mDeclReqAttribUIFns
};


