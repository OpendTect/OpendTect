#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uigroup.h"

#include "datapointset.h"
#include "emobject.h"

class uiBatchJobDispatcherSel;
class uiGenInput;
class uiIOObjSel;


/*! \brief Create isochron as attribute of horizon */

mExpClass(uiEarthModel) uiIsochronMakerGrp : public uiGroup
{
mODTextTranslationClass(uiIsochronMakerGrp)
public:
			uiIsochronMakerGrp(uiParent*,EM::ObjectID);
			~uiIsochronMakerGrp();

    bool		chkInputFlds();
    bool		fillPar(IOPar&);
    BufferString	getHorNm(EM::ObjectID);
    const char*		attrName() const;

protected:
    uiIOObjSel*		basesel_		= nullptr;
    uiIOObjSel*		horsel_;
    uiGenInput*		attrnmfld_;
    uiGenInput*		msecsfld_		= nullptr;
    EM::ObjectID	horid_;
    RefMan<EM::EMObject> baseemobj_;

    void		toHorSel(CallBacker*);
};


mExpClass(uiEarthModel) uiIsochronMakerBatch : public uiDialog
{
mODTextTranslationClass(uiIsochronMakerBatch)
public:
			uiIsochronMakerBatch(uiParent*);
			~uiIsochronMakerBatch();

protected:
    bool		prepareProcessing();
    bool		fillPar();
    bool		acceptOK(CallBacker*) override;

    uiIsochronMakerGrp*		grp_;
    uiBatchJobDispatcherSel*	batchfld_;
    bool			isoverwrite_;
};


mExpClass(uiEarthModel) uiIsochronMakerDlg : public uiDialog
{
mODTextTranslationClass(uiIsochronMakerDlg)
public:
			uiIsochronMakerDlg(uiParent*,EM::ObjectID);
			~uiIsochronMakerDlg();

    const char*		attrName() const	{ return grp_->attrName(); }
    const DataPointSet&	getDPS()		{ return *dps_; }

protected:
    bool		acceptOK(CallBacker*) override;
    bool		doWork();

    uiIsochronMakerGrp*		grp_;
    RefMan<DataPointSet>	dps_;
};
