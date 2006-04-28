#ifndef uifingerprintattrib_h
#define uifingerprintattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uifingerprintattrib.h,v 1.5 2006-04-28 10:00:47 cvshelene Exp $
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
class uiRadioButton;
class uiToolButton;
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
    uiRadioButton*	refposbut_;
    uiRadioButton*	picksetbut_;
    uiToolButton*	getposbut_;
    uiGenInput*		statsfld_;
    uiGenInput*		refposzfld_;
    uiStepOutSel*	refposfld_;
    uiIOObjSel*		picksetfld_;
   
    CtxtIOObj&		ctio_;
    ObjectSet<uiAttrSel> attribflds_;

    void		insertRowCB(CallBacker*);
    void		deleteRowCB(CallBacker*);
    void		initTable(int);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    
    void                getPosPush(CallBacker*);
    void                calcPush(CallBacker*);
    void		refSel(CallBacker*);
    EngineMan*          createEngineMan(int&);
    void                showValues(BinIDValueSet*,int);
};

#endif
