#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uidialog.h"
#include "mmbatchjobdispatch.h"
#include "clusterjobdispatch.h"

class IOObj;

class uiBatchJobDispatcherSel;
class uiIOObjSel;
class uiSeisSubSel;
class uiPushButton;
class uiVelocityDesc;


namespace VolProc
{

class Chain;

mExpClass(uiVolumeProcessing) uiBatchSetup : public uiDialog
{ mODTextTranslationClass(uiBatchSetup);

public:
			uiBatchSetup(uiParent*,
				     const IOObj* setupsel=0,
				     bool is2d=false);
			~uiBatchSetup();

    void		setIOObj(const IOObj*);

protected:

    bool		prepareProcessing();
    bool		fillPar();
    bool		retrieveChain();

    uiIOObjSel*		setupsel_;
    uiPushButton*	editsetup_;
    uiSeisSubSel*	subsel_;
    uiIOObjSel*		outputsel_;
    Chain*		chain_;
    uiBatchJobDispatcherSel* batchfld_;
    bool		is2d_;

    void		setupSelCB(CallBacker*);
    void		editPushCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;
};

} // namespace VolProc


namespace Batch
{

class VolMMProgDef : public MMProgDef
{
public:
			VolMMProgDef() : MMProgDef( "od_SeisMMBatch" )	{}
    bool		isSuitedFor(const char*) const override;
    bool		canHandle(const JobSpec&) const override;
    static const char*	sKeyNeedsFullVolYN()	{ return "NeedsFullVol"; }
    bool		canResume(const JobSpec&) const override;
};

class VolClusterProgDef : public ClusterProgDef
{
public:
			VolClusterProgDef() {}

    bool		isSuitedFor(const char*) const override;
    bool		canHandle(const JobSpec&) const override;
    static const char*	sKeyNeedsFullVolYN()	{ return "NeedsFullVol"; }
};


} // namespace Batch
