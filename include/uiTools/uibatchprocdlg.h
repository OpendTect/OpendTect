#ifndef uibatchprocdlg_h
#define uibatchprocdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		July 2015
 RCS:		$Id $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "batchjobdispatch.h"

class uiBatchJobDispatcherSel;
namespace Attrib { class DescSet; }
class NLAModel;

mExpClass(uiTools) uiBatchProcDlg : public uiDialog
{ mODTextTranslationClass(uiBatchProcDlg);
public:
			uiBatchProcDlg(uiParent*,const uiString& dlgnm,
				       bool optional,
				       const Batch::JobSpec::ProcType& pt);

protected:
    virtual bool		prepareProcessing() { return true; }
    virtual void		getJobName(BufferString& jobnm) const;
    virtual bool		fillPar(IOPar&)		=0;

    bool			acceptOK(CallBacker*);
    void			setProgName(const char*);

    uiGroup*			pargrp_;
    uiGroup*			batchgrp_;
    uiBatchJobDispatcherSel*	batchjobfld_;
};

#endif

