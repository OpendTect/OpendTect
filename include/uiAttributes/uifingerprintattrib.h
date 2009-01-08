#ifndef uifingerprintattrib_h
#define uifingerprintattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uifingerprintattrib.h,v 1.16 2009-01-08 08:50:11 cvsranojay Exp $
________________________________________________________________________

-*/

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
class uiPushButton;
class uiButtonGroup;
class uiSpinBox;
class uiLabeledComboBox;
class BinIDValueSet;

class uiFPAdvancedDlg;
class calcFingParsObject;

/*! \brief FingerPrint Attribute description editor */

mClass uiFingerPrintAttrib : public uiAttrDescEd
{
public:

			uiFingerPrintAttrib(uiParent*,bool);
			~uiFingerPrintAttrib();

protected:

    uiTable*            table_;
    uiButtonGroup*      refgrp_;
    uiRadioButton*	refposbut_;
    uiRadioButton*	picksetbut_;
    uiToolButton*	getposbut_;
    uiPushButton*	sel2dbut_;
    uiGenInput*		statsfld_;
    uiGenInput*		refposfld_;
    uiGenInput*		refpos2dfld_;
    uiGenInput*		refposzfld_;
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

    			mDeclReqAttribUIFns
};


#endif
