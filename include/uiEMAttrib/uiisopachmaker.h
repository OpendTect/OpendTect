#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2008
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "dbkey.h"

class uiBatchJobDispatcherSel;
class uiIOObjSel;
class uiGenInput;
class CtxtIOObj;
class DataPointSet;
namespace EM { class Object; }


/*! \brief Create isochron as attribute of horizon */

mExpClass(uiEMAttrib) uiIsochronMakerGrp : public uiGroup
{ mODTextTranslationClass(uiIsochronMakerGrp);
public:
			uiIsochronMakerGrp(uiParent*,const DBKey&);
			~uiIsochronMakerGrp();

    bool		chkInputFlds();
    bool		fillPar(IOPar&);
    BufferString	getHorNm(const DBKey&);
    const char*		attrName() const;

protected:
    uiIOObjSel*		basesel_;
    uiIOObjSel*		horsel_;
    uiGenInput*		attrnmfld_;
    uiGenInput*		msecsfld_;
    CtxtIOObj&		basectio_;
    CtxtIOObj&		ctio_;
    DBKey		horid_;
    EM::Object*		baseemobj_;

    void		toHorSel(CallBacker*);
};


mExpClass(uiEMAttrib) uiIsochronMakerBatch : public uiDialog
{ mODTextTranslationClass(uiIsochronMakerBatch);
public:

			uiIsochronMakerBatch(uiParent*);
protected:
    bool		prepareProcessing();
    bool		fillPar();
    bool		acceptOK();

    uiIsochronMakerGrp*		grp_;
    uiBatchJobDispatcherSel*	batchfld_;
    bool			isoverwrite_;
};


mExpClass(uiEMAttrib) uiIsochronMakerDlg : public uiDialog
{ mODTextTranslationClass(uiIsochronMakerDlg);
public:
			uiIsochronMakerDlg(uiParent*,const DBKey&);
			~uiIsochronMakerDlg();

    const char*		attrName() const	{ return grp_->attrName(); }
    const DataPointSet&	getDPS()		{ return *dps_; }

protected:
    bool		acceptOK();
    bool		doWork();

    uiIsochronMakerGrp*	grp_;
    DataPointSet*	dps_;
};
