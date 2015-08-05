/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	nageswara
 Date:		July 2015
________________________________________________________________________

-*/

#include "uibatchprocdlg.h"

#include "batchjobdispatch.h"
#include "errmsg.h"
#include "uibatchjobdispatchersel.h"

uiBatchProcDlg::uiBatchProcDlg( uiParent* p, const uiString& dlgnm,
				bool optional,
				const Batch::JobSpec::ProcType& pt )
    : uiDialog(p,Setup(dlgnm, mNoDlgTitle, mNoHelpKey).modal(false))
{
    pargroup_ = new uiGroup( this, "Parmeters group" );
    batchgrp_ = new uiGroup( this, "Batch group" );
    batchgrp_->attach( alignedBelow, pargroup_ );
    batchjobfld_ = new uiBatchJobDispatcherSel( batchgrp_, optional, pt );
    batchgrp_->setHAlignObj( batchjobfld_ );
}


const char* uiBatchProcDlg::jobName() const
{
    return "Batch_processing";
}


bool uiBatchProcDlg::acceptOK( CallBacker* )
{
    batchjobfld_->setJobName( jobName() );
    if ( !prepareProcessing() )
	return false;

    IOPar& par = batchjobfld_->jobSpec().pars_;
    if ( !fillPar(par) )
	return false;

    return batchjobfld_->start();
}
