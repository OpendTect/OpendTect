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
#include "uimsg.h"

uiBatchProcDlg::uiBatchProcDlg( uiParent* p, const uiString& dlgnm,
				bool optional,
				const Batch::JobSpec::ProcType& pt )
    : uiDialog(p,Setup(dlgnm, mNoDlgTitle, mNoHelpKey).modal(false))
{
    setCtrlStyle( RunAndClose );

    pargrp_ = new uiGroup( this, "Parmeters group" );
    batchgrp_ = new uiGroup( this, "Batch group" );
    batchgrp_->attach( alignedBelow, pargrp_ );
    batchjobfld_ = new uiBatchJobDispatcherSel( batchgrp_, optional, pt );
    batchgrp_->setHAlignObj( batchjobfld_ );
}


void uiBatchProcDlg::getJobName( BufferString& jobnm ) const
{
    jobnm = "Batch_processing";
}


bool uiBatchProcDlg::acceptOK( CallBacker* )
{
    if ( !prepareProcessing() )
	return false;

    IOPar& par = batchjobfld_->jobSpec().pars_;
    par.setEmpty();
    BufferString jobnm;
    getJobName( jobnm );
    batchjobfld_->setJobName( jobnm );
    if ( !fillPar(par) )
	return false;

    if ( !batchjobfld_->start() )
	uiMSG().error( uiStrings::sBatchProgramFailedStart() );

    return false;
}


void uiBatchProcDlg::setProgName( const char* prognm )
{
    batchjobfld_->jobSpec().prognm_ = prognm;
}
