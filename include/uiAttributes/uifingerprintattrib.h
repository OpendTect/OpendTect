#ifndef uifingerprintattrib_h
#define uifingerprintattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uifingerprintattrib.h,v 1.3 2006-04-25 14:47:46 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "uidialog.h"
#include "position.h"

class CtxtIOObj;
class uiAttrSel;
class uiTable;
class uiStepOutSel;
class uiIOObjSel;
class uiGenInput;
class uiPushButton;
class uiToolButton;
class uiRadioButton;
class uiButtonGroup;
class BinIDValueSet;
namespace Attrib { class EngineMan; }


/*! \brief FingerPrint Attribute description editor */

class uiFingerPrintAttrib : public uiAttrDescEd
{
public:

			uiFingerPrintAttrib(uiParent*);
			~uiFingerPrintAttrib();

    void		set2D(bool);

protected:

    uiTable*            table_;
    uiButtonGroup*      refgrp_;
    uiRadioButton*	manualbut_;
    uiRadioButton*	refposbut_;
    uiRadioButton*	picksetbut_;
    uiGenInput*		statsfld_;
    uiGenInput*		refposzfld_;
    uiStepOutSel*	refposfld_;
    uiToolButton*       getposbut_;
    uiPushButton*       calcbut_;
    uiIOObjSel*		picksetfld_;
   
    CtxtIOObj&		ctio_;
    ObjectSet<uiAttrSel> attribflds_;

    void		insertRowCB(CallBacker*);
    void		deleteRowCB(CallBacker*);
    void		initTable();

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    
    void                getPosPush(CallBacker*);
    void                calcPush(CallBacker*);
    void		refSel(CallBacker*);
    EngineMan*          createEngineMan();
    void                showValues(BinIDValueSet*);
};

#endif
