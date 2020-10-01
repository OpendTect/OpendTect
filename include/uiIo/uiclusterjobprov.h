#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2009
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uibatchjobdispatcherlauncher.h"
#include "uidialog.h"
#include "clusterjobdispatch.h"
#include "dbkey.h"

class InlineSplitJobDescProv;
class uiGenInput;
class uiFileSel;
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
    Batch::ID		batchid_;

    uiGenInput*		nrinlfld_;
    uiLabel*		nrjobsfld_;
    uiFileSel*		parfilefld_;
    uiFileSel*		tmpstordirfld_;
    uiFileSel*		scriptdirfld_;
    uiGenInput*		cmdfld_;

    void		nrJobsCB(CallBacker*);
    bool		acceptOK();

    bool		createJobScripts(const char*);
    const char*		getOutPutIDKey() const;
    DBKey		getTmpID(const char*) const;
};


namespace Batch
{
mExpClass(uiIo) SimpleClusterProgDef : public ClusterProgDef
{
public:
			SimpleClusterProgDef() {}

    bool		isSuitedFor(const char*) const;
    bool		canHandle(const JobSpec&) const;
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

    virtual bool	go(uiParent*,Batch::ID* =nullptr);

protected:

    virtual Batch::JobDispatcher&	gtDsptchr();
    Batch::ClusterJobDispatcher&	jd_;

};
