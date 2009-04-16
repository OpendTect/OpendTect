#ifndef uifingerprintattrib_h
#define uifingerprintattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uifingerprintattrib.h,v 1.17 2009-04-16 14:45:05 cvsbert Exp $
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
class uiSeis2DLineSel;
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
    calcFingParsObject*	calcobj_;

    void		insertRowCB(CallBacker*);
    void		deleteRowCB(CallBacker*);
    void		initTable(int);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    BinIDValueSet*	createValuesBinIDSet(BufferString&) const;
    BinID		get2DRefPos() const;

    void                getPosPush(CallBacker*);
    void                calcPush(CallBacker*);
    void                getAdvancedPush(CallBacker*);
    void		refSel(CallBacker*);

    virtual bool	areUIParsOK();

    			mDeclReqAttribUIFns
};


#endif
