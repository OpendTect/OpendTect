/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uibatchlaunch.h"

#include "uiclusterjobprov.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uibatchjobdispatchersel.h"

#include "jobdescprov.h"
#include "settings.h"
#include "envvars.h"

static const char* sKeyClusterProc = "dTect.Enable Cluster Processing";
static const char* sKeyClusterProcEnv = "DTECT_CLUSTER_PROC";

static bool enabClusterProc()
{
    bool enabclusterproc = false;
    const bool hassetting =
	Settings::common().getYN( sKeyClusterProc, enabclusterproc );
    if ( !hassetting )
	enabclusterproc = GetEnvVarYN( sKeyClusterProcEnv );
    return enabclusterproc;
}


uiProcSettings::uiProcSettings( uiParent* p )
    : uiDialog(p,Setup("Processing settings",mNoDlgTitle,"103.2.27"))
{
    const int nrinl = InlineSplitJobDescProv::defaultNrInlPerJob();
    nrinlfld_ = new uiGenInput( this, "Default number of inlines per job",
				IntInpSpec(nrinl,1,10000) );

    const bool enabclusterproc = enabClusterProc();
    clusterfld_ = new uiGenInput( this, "Enable cluster processing",
				  BoolInpSpec(enabclusterproc) );
    clusterfld_->attach( alignedBelow, nrinlfld_ );
}


bool uiProcSettings::acceptOK( CallBacker* )
{
    InlineSplitJobDescProv::setDefaultNrInlPerJob( nrinlfld_->getIntValue() );
    Settings::common().setYN( sKeyClusterProc, clusterfld_->getBoolValue() );
    Settings::common().write( false );
    return true;
}


uiStartBatchJobDialog::uiStartBatchJobDialog( uiParent* p )
    : uiDialog(p,Setup("Start a batch job",mNoDlgTitle,"101.2.1"))
{
    uiLabeledListBox* llb = new uiLabeledListBox( this, "Stored Batch Job" );
    jobsfld_ = llb->box();
    batchfld_ = new uiBatchJobDispatcherSel( this, false,
	    				     Batch::JobSpec::Attrib );
    batchfld_->attach( alignedBelow, llb );
    resumefld_ = new uiGenInput( this, "Use already processed data",
	    		BoolInpSpec(false,"Yes","No (start from scratch)") );
    resumefld_->attach( alignedBelow, batchfld_ );
}


bool uiStartBatchJobDialog::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: implement" );
    return true;
}
