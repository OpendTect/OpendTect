#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	H. Payraudeau
 Date:		February 2006
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"
#include "position.h"
#include "multiid.h"

class CtxtIOObj;
class uiAttrSel;
class uiTable;
class uiLabel;
class uiIOObjSel;
class uiGenInput;
class uiRadioButton;
class uiToolButton;
class uiButtonGroup;
class uiSeis2DLineSel;
class BinIDValueSet;
class PickRetriever;

class uiFPAdvancedDlg;
class calcFingParsObject;

/*! \brief FingerPrint Attribute description editor */

mExpClass(uiAttributes) uiFingerPrintAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiFingerPrintAttrib);
public:

			uiFingerPrintAttrib(uiParent*,bool);
			~uiFingerPrintAttrib();

protected:

    uiTable*		table_;
    uiButtonGroup*	refgrp_;
    uiRadioButton*	refposbut_;
    uiRadioButton*	picksetbut_;
    uiToolButton*	getposbut_;
    uiGenInput*		statsfld_;
    uiGenInput*		refposfld_;
    uiGenInput*		refpos2dfld_;
    uiGenInput*		refposzfld_;
    uiIOObjSel*		picksetfld_;
    uiLabel*		manlbl_;
    uiSeis2DLineSel*	linefld_;
   
    CtxtIOObj&		ctio_;
    ObjectSet<uiAttrSel> attribflds_;

    uiFPAdvancedDlg*	advanceddlg_;
    calcFingParsObject* calcobj_;

    void		insertRowCB(CallBacker*);
    void		deleteRowCB(CallBacker*);
    void		initTable(int);

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    BinIDValueSet*	createValuesBinIDSet(uiString&) const;
    BinID		get2DRefPos() const;

    PickRetriever*	pickretriever_;
    void		getPosPush(CallBacker*);
    void		pickRetrieved(CallBacker*);

    void		calcPush(CallBacker*);
    void		getAdvancedPush(CallBacker*);
    void		refSel(CallBacker*);

    bool		areUIParsOK() override;

			mDeclReqAttribUIFns
};


