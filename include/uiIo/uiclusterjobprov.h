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
class uiFileInput;
class uiLabel;
namespace Batch { class ClusterJobDispatcher; }


mExpClass(uiIo) uiClusterJobProv : public uiDialog
{ mODTextTranslationClass(uiClusterJobProv);
public:
			uiClusterJobProv(uiParent* p,const IOPar& iop,
					 const char* prog,const char* parfnm);
			~uiClusterJobProv();

    static const char*	sKeySeisOutIDKey();
    static const char*	sKeyOutputID();

protected:

    InlineSplitJobDescProv*	jobprov_;
    IOPar&		iopar_;
    const char*		prognm_;
    BufferString	tempstordir_;

    uiGenInput*		nrinlfld_;
    uiLabel*		nrjobsfld_;
    uiFileInput*	parfilefld_;
    uiFileInput*	tmpstordirfld_;
    uiFileInput*	scriptdirfld_;
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

    virtual bool	go(uiParent*);

protected:

    virtual Batch::JobDispatcher&	gtDsptchr();
    Batch::ClusterJobDispatcher&	jd_;

};
