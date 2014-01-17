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
class uiGenInput;
class uiButton;
class uiCheckBox;

class uiBatchJobDispatcherLauncher;


/*!\brief lets user select a batch job dispatcher suited for the job */

mExpClass(uiTools) uiBatchJobDispatcherSel : public uiGroup
{
public:

			uiBatchJobDispatcherSel(uiParent*,bool optional,
					Batch::JobSpec::ProcType pt
						=Batch::JobSpec::NonODBase);

    void		setJobSpec(const Batch::JobSpec&);
			//!< must be done at every change
			//!< the spec will determine what user can select

    Batch::JobSpec&	jobSpec()		{ return jobspec_; };
    const char*		selected() const;
    const char*		selectedInfo() const;

    bool		wantBatch() const;	//! can be false if isoptional
    bool		start();

    Notifier<uiBatchJobDispatcherSel> selectionChange;

protected:

    uiGenInput*		selfld_;
    uiCheckBox*		dobatchbox_;
    uiButton*		optsbut_;

    Batch::JobSpec	jobspec_;
    ObjectSet<uiBatchJobDispatcherLauncher> uidispatchers_;

    int			selIdx() const;

    void		initFlds(CallBacker*);
    void		selChg(CallBacker*);
    void		fldChck(CallBacker*);
    void		optsPush(CallBacker*);

    bool		noLaunchersAvailable() const	{ return !optsbut_; }
};


#endif
