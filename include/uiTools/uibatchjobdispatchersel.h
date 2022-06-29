#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2014
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "batchjobdispatch.h"

class uiBatchJobDispatcherLauncher;
class uiButton;
class uiCheckBox;
class uiGenInput;
class IOObj;


/*!\brief Lets user select a batch job dispatcher suited for the job */

mExpClass(uiTools) uiBatchJobDispatcherSel : public uiGroup
{ mODTextTranslationClass(uiBatchJobDispatcherSel)
public:

    mUseType( Batch,	JobSpec );
    mUseType( JobSpec,	ProcType );

			uiBatchJobDispatcherSel(uiParent*,bool optional,
					ProcType pt=JobSpec::NonODBase,
					OS::LaunchType type=OS::Batch);
			uiBatchJobDispatcherSel(uiParent*,bool optional,
						const JobSpec&);

    void		jobSpecUpdated();
    void		setJobSpec(const JobSpec&);
    void		setJobName(const char*);
    void		setWantBatch(bool);	//! useful if isoptional

    JobSpec&		jobSpec()		{ return jobspec_; }
    uiString		selected() const;
    const uiString	selectedInfo() const;
    uiBatchJobDispatcherLauncher* selectedLauncher();

    bool		wantBatch() const;	//! can be false if isoptional
    bool		start(Batch::ID* =nullptr);
    bool		saveProcPars(const IOObj&) const;

    Notifier<uiBatchJobDispatcherSel> selectionChange;
    Notifier<uiBatchJobDispatcherSel> checked;	//! only when isoptional

protected:

    uiGenInput*		selfld_			= nullptr;
    uiCheckBox*		dobatchbox_		= nullptr;
    uiButton*		optsbut_;

    BufferString	jobname_		= "batch_processing";
    JobSpec		jobspec_;
    ObjectSet<uiBatchJobDispatcherLauncher> uidispatchers_;

    void		init(bool optional);
    int			selIdx() const;
    BufferStringSet	getActiveProgramList() const;

    void		initFlds(CallBacker*);
    void		selChg(CallBacker*);
    void		fldChck(CallBacker*);
    void		optsPush(CallBacker*);
    void		removeBatchProcess(CallBacker*);
    void		triggerRemove(CallBacker*);
    bool		noLaunchersAvailable() const	{ return !optsbut_; }

};
