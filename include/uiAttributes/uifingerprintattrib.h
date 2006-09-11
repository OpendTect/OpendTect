#ifndef uifingerprintattrib_h
#define uifingerprintattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uifingerprintattrib.h,v 1.10 2006-09-11 07:04:12 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "uidialog.h"
#include "position.h"
#include "ranges.h"
#include "runstat.h"
#include "multiid.h"

class CtxtIOObj;
class uiAttrSel;
class uiTable;
class uiLabel;
class uiStepOutSel;
class uiIOObjSel;
class uiGenInput;
class uiRadioButton;
class uiToolButton;
class uiPushButton;
class uiButtonGroup;
class uiSpinBox;
class uiLabeledComboBox;
class BinIDValueSet;

class uiFPAdvancedDlg;
class calcFingParsObject;

/*! \brief FingerPrint Attribute description editor */

class uiFingerPrintAttrib : public uiAttrDescEd
{
public:
    static void		initClass();
			uiFingerPrintAttrib(uiParent*);
			~uiFingerPrintAttrib();

    void		set2D(bool);
    const char*		getAttribName() const;

protected:
    static uiAttrDescEd* createInstance(uiParent*);

    uiTable*            table_;
    uiButtonGroup*      refgrp_;
    uiRadioButton*	refposbut_;
    uiRadioButton*	picksetbut_;
    uiToolButton*	getposbut_;
    uiPushButton*	sel2dbut_;
    uiGenInput*		statsfld_;
    uiGenInput*		refposzfld_;
    uiStepOutSel*	refposfld_;
    uiIOObjSel*		picksetfld_;
    uiLabel*		manlbl_;
    uiGenInput*		linesetfld_;
    uiLabeledComboBox*	linefld_;
   
    CtxtIOObj&		ctio_;
    MultiID		lsid_;
    ObjectSet<uiAttrSel> attribflds_;

    uiFPAdvancedDlg*	advanceddlg_;
    calcFingParsObject*	calcobj_;

    void		insertRowCB(CallBacker*);
    void		deleteRowCB(CallBacker*);
    void		initTable(int);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    BinIDValueSet*	createValuesBinIDSet(BufferString&) const;
    void		get2DLineSetName(const MultiID&,BufferString&) const;
    void		useLineSetID(const BufferString&);
    BinID		get2DRefPos() const;
    
    void                getPosPush(CallBacker*);
    void                calcPush(CallBacker*);
    void                getAdvancedPush(CallBacker*);
    void		refSel(CallBacker*);
    void		fillIn2DPos(CallBacker*);

    virtual bool	areUIParsOK();
};

#endif
