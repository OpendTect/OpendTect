#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uibatchjobdispatcherlauncher.h"
#include "uidialog.h"
#include "clusterjobdispatch.h"
#include "multiid.h"

class InlineSplitJobDescProv;
class uiGenInput;
class uiFileInput;
class uiLabel;
namespace Batch { class ClusterJobDispatcher; }


mExpClass(uiIo) uiClusterJobProv : public uiDialog
{ mODTextTranslationClass(uiClusterJobProv);
public:
			uiClusterJobProv(uiParent* p,const IOPar& iop,
					 const char* prog,const char* parfnm,
					 Batch::ID* =nullptr);
			~uiClusterJobProv();

    static const char*	sKeySeisOutIDKey();
    static const char*	sKeyOutputID();

protected:

    InlineSplitJobDescProv*	jobprov_;
    IOPar&		iopar_;
    const char*		prognm_;
    BufferString	tempstordir_;
    BufferString	scriptdir_;
    BufferString	parfnm_;
    Batch::ID		batchid_;

    uiGenInput*		nrinlfld_;
    uiLabel*		nrjobsfld_;
    uiGenInput*		cmdfld_;

    uiGenInput*		datarootfld_;
    uiGenInput*		instdirfld_;

    void		nrJobsCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    bool		createJobScripts(const char*,const char*);
    const char*		getOutPutIDKey() const;
    MultiID		getTmpID(const char*) const;
};


namespace Batch
{
mExpClass(uiIo) SimpleClusterProgDef : public ClusterProgDef
{
public:
			SimpleClusterProgDef() {}

    bool		isSuitedFor(const char*) const override;
    bool		canHandle(const JobSpec&) const override;
};
}


mExpClass(uiIo) uiClusterJobDispatcherLauncher
					: public uiBatchJobDispatcherLauncher
{ mODTextTranslationClass(uiClusterJobDispatcherLauncher)
public:
			uiClusterJobDispatcherLauncher(Batch::JobSpec&);
			~uiClusterJobDispatcherLauncher();

    mDefaultFactoryInstantiation1Param(uiBatchJobDispatcherLauncher,
				       uiClusterJobDispatcherLauncher,
				       Batch::JobSpec&,
				       "Cluster Processing",
				       tr("Cluster Processing"));

    bool	go(uiParent*,Batch::ID* =nullptr) override;

protected:

    Batch::JobDispatcher&		gtDsptchr() override;
    Batch::ClusterJobDispatcher&	jd_;

};
