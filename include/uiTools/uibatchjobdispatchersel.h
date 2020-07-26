#ifndef uibatchjobdispatchersel_h
#define uibatchjobdispatchersel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2014
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "batchjobdispatch.h"
#include "netservice.h"

class uiBatchJobDispatcherLauncher;
class uiButton;
class uiCheckBox;
class uiGenInput;
class IOObj;


/*!\brief Lets user select a batch job dispatcher suited for the job */

mExpClass(uiTools) uiBatchJobDispatcherSel : public uiGroup
{ mODTextTranslationClass(uiBatchJobDispatcherSel)
public:

			uiBatchJobDispatcherSel(uiParent*,bool optional,
					Batch::JobSpec::ProcType pt
				    =Batch::JobSpec::NonODBase,
				    OS::LaunchType type=OS::Batch);
			uiBatchJobDispatcherSel(uiParent*,bool optional,
				    const Batch::JobSpec&,
				    OS::LaunchType type=OS::Batch);

    void		jobSpecUpdated();
    void		setJobSpec(const Batch::JobSpec&);
    void		setJobName(const char*);
    void		setWantBatch(bool);	//! useful if isoptional

    Batch::JobSpec&	jobSpec()		{ return jobspec_; }
    uiString		selected() const;
    const uiString	selectedInfo() const;
    uiBatchJobDispatcherLauncher* selectedLauncher();

    bool		wantBatch() const;	//! can be false if isoptional
    bool		start();
    bool		saveProcPars(const IOObj&) const;

    Notifier<uiBatchJobDispatcherSel> selectionChange;
    Notifier<uiBatchJobDispatcherSel> checked;	//! only when isoptional

protected:

    uiGenInput*		selfld_;
    uiCheckBox*		dobatchbox_;
    uiButton*		optsbut_;

    BufferString	jobname_;
    Batch::JobSpec	jobspec_;
    OS::LaunchType	launchtype_;
    ObjectSet<uiBatchJobDispatcherLauncher> uidispatchers_;

    void		init(bool optional);
    int			selIdx() const;

    void		initFlds(CallBacker*);
    void		selChg(CallBacker*);
    void		fldChck(CallBacker*);
    void		optsPush(CallBacker*);

    bool		noLaunchersAvailable() const	{ return !optsbut_; }
};


#endif
