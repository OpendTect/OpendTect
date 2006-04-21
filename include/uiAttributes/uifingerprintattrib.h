#ifndef uifingerprintattrib_h
#define uifingerprintattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uifingerprintattrib.h,v 1.2 2006-04-21 08:13:22 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "uidialog.h"
#include "position.h"

class uiAttrSel;
class uiTable;
class uiStepOutSel;
class uiGenInput;
class uiPushButton;
class uiToolButton;
class BinIDValueSet;
namespace Attrib { class EngineMan; }


/*! \brief FingerPrint Attribute description editor */

class uiFingerPrintAttrib : public uiAttrDescEd
{
public:

			uiFingerPrintAttrib(uiParent*);

    void		set2D(bool);

protected:

    uiTable*            table_;
    uiGenInput*		usereffld_;
    uiGenInput*		refposzfld_;
    uiStepOutSel*	refposfld_;
    uiToolButton*       getposbut_;
    uiPushButton*       calcbut_;
    
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
    void		isRefSel(CallBacker*);
    EngineMan*          createEngineMan();
    void                showValues(BinIDValueSet*);
};

#endif
