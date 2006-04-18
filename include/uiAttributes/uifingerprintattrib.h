#ifndef uifingerprintattrib_h
#define uifingerprintattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uifingerprintattrib.h,v 1.1 2006-04-18 11:09:05 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "uidialog.h"
#include "position.h"

class uiAttrSel;
class uiTable;
class uiStepOutSel;
class uiPushButton;
class uiFingerPrintGetValDlg;


/*! \brief FingerPrint Attribute description editor */

class uiFingerPrintAttrib : public uiAttrDescEd
{
public:

			uiFingerPrintAttrib(uiParent*);

    void		set2D(bool);

protected:

    uiTable*            table_;
    uiPushButton*	getvalbut_;
    
    ObjectSet<uiAttrSel> attribflds_;

    void		getValPush(CallBacker*);
    void		valDlgClosed(CallBacker*);
    void		insertRowCB(CallBacker*);
    void		deleteRowCB(CallBacker*);
    void		initTable();

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    
    TypeSet<float> 	values_;
    uiFingerPrintGetValDlg* getvaldlg_;
};


class uiGenInput;
class uiToolButton;
class BinIDValueSet;
namespace Attrib { class EngineMan; }

class uiFingerPrintGetValDlg : public uiDialog
{
public:
			uiFingerPrintGetValDlg(uiParent*);

    void		set2D(bool);
    void                setRefZ( float z )		{ refposz_ = z; }
    void                setRefBinID( const BinID& bid )	{ refpos_ = bid; }
    void		setDescSet( DescSet* ds )	{ attrset_ = ds; }
    float               getRefZ() const			{ return refposz_; }
    BinID               getRefBinID() const		{ return refpos_; }
    TypeSet<float>	getValues()			{ return values_; }

protected:

    EngineMan*		createEngineMan();
    void                getPosPush(CallBacker*);
    void                calcPush(CallBacker*);
    void		saveValues(BinIDValueSet*);
    uiStepOutSel*	refposfld_;
    uiGenInput*		refposzfld_;
    uiToolButton*	getposbut_;
    uiPushButton*	calcbut_;

    BinID		refpos_;
    float		refposz_;
    DescSet*		attrset_;
    TypeSet<float>	values_;
};

#endif
