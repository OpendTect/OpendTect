#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "batchjobdispatch.h"

class uiBatchJobDispatcherSel;


mExpClass(uiTools) uiBatchProcDlg : public uiDialog
{ mODTextTranslationClass(uiBatchProcDlg);
public:

    mUseType( Batch::JobSpec,	ProcType );

				uiBatchProcDlg(uiParent*,const uiString&,
					       bool optional,ProcType);

    Batch::ID			getLastID() const   { return batchid_; }

protected:

    virtual bool		prepareProcessing() { return true; }
    virtual void		getJobName(BufferString&) const;
    virtual bool		fillPar(IOPar&)		= 0;

    bool			acceptOK(CallBacker*) override;
    void			setProgName(const char*);

    uiGroup*			pargrp_;
    uiGroup*			batchgrp_;
    uiBatchJobDispatcherSel*	batchjobfld_;
    Batch::ID			batchid_ = Batch::JobDispatcher::getInvalid();

};
