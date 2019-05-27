/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : May 2019
-*/

#include "uifirewallprocsetterdlg.h"
#include "uigeninput.h"
#include "uilistbox.h"

#include "oscommand.h"

uiFirewallProcSetter::uiFirewallProcSetter( uiParent* p )
    : uiDialog(p,Setup(tr("Set Firewall Program Rules"),mNoDlgTitle,
					mODHelpKey(mBatchHostsDlgHelpID)))
{
    uiListBox::Setup su;
    su.lbl( tr("OpendTect Executables") );
    su.cm( OD::ChoiceMode::ChooseAtLeastOne );
    odproclistbox_ = new uiListBox( this, su );
    odproclistbox_->setHSzPol( uiObject::SzPolicy::WideMax );
    static BufferStringSet odprocnms;
    odprocnms.add( "od_remoteservice" );
    odprocnms.add( "od_SeisMMBatch" );
    odproclistbox_->addItems( odprocnms );

    su.lbl( tr( "Python Executables" ) );
    pythonproclistbox_ = new uiListBox( this, su );
    pythonproclistbox_->setHSzPol( uiObject::SzPolicy::WideMax );
    pythonproclistbox_->attach( alignedBelow, odproclistbox_ );
    static BufferStringSet pythonprocnms;
    pythonprocnms.add( "python" );
    pythonproclistbox_->addItems( pythonprocnms );

    addremfld_ = new uiGenInput( this, uiStrings::sAction(),BoolInpSpec(true,
				    uiStrings::sAdd(), uiStrings::sRemove()) );
    addremfld_->attach( alignedBelow, pythonproclistbox_ );
}


uiFirewallProcSetter::~uiFirewallProcSetter()
{
}


bool uiFirewallProcSetter::acceptOK()
{
    BufferStringSet odchosenproc;
    odproclistbox_->getChosen( odchosenproc );

    BufferStringSet pythonchosenproc;
    odproclistbox_->getChosen( pythonchosenproc );

    if ( odchosenproc.isEmpty() && pythonchosenproc.isEmpty() )
	return false;

    BufferStringSet allprocs;
    allprocs.add( odchosenproc.getDispString() );
    allprocs.add( pythonchosenproc.getDispString() );

    BufferString cmd = "od_Setup_Firewall.exe";
    cmd.addSpace();
    if ( addremfld_->getBoolValue() )
	cmd.add( "--add" );
    else
	cmd.add( "--remove" );

    cmd.addSpace();

    for ( int idx=0; idx<allprocs.size(); idx++ )
    {
	BufferString fincmd = cmd;
	if ( idx == 0 )
	    fincmd.add("o");
	else
	    fincmd.add("p");
	fincmd.addSpace().add( allprocs.get(idx) );

	OS::MachineCommand mchcmd( fincmd );
    }

    return true;
}
