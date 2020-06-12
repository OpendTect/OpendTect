/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : May 2019
-*/

#include "uifirewallprocsetterdlg.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"

#include "dirlist.h"
#include "filepath.h"
#include "keystrs.h"
#include "pythonaccess.h"
#include "oscommand.h"
#include "settings.h"

uiFirewallProcSetter::uiFirewallProcSetter( uiParent* p )
    : uiDialog(p,Setup(tr("Set Firewall Program Rules"),mNoDlgTitle,
					mODHelpKey(mBatchHostsDlgHelpID)))
{
    uiListBox::Setup su;
    su.lbl( tr("OpendTect Executables") );
    su.cm( OD::ChoiceMode::ChooseAtLeastOne );
    odproclistbox_ = new uiListBox( this, su );
    odproclistbox_->setHSzPol( uiObject::SzPolicy::WideMax );
    BufferStringSet odprocnms;
    odprocnms.add( "od_remoteservice" );
    odprocnms.add( "od_SeisMMBatch" );
    odproclistbox_->addItems( odprocnms );

    su.lbl( tr( "Python Executables" ) );
    pythonproclistbox_ = new uiListBox( this, su );
    pythonproclistbox_->setHSzPol( uiObject::SzPolicy::WideMax );
    pythonproclistbox_->attach( alignedBelow, odproclistbox_ );

    pythonproclistbox_->addItems( getPythonExecList() );

    addremfld_ = new uiGenInput( this, uiStrings::sAction(),BoolInpSpec(true,
				    uiStrings::sAdd(), uiStrings::sRemove()) );
    addremfld_->attach( alignedBelow, pythonproclistbox_ );
}


uiFirewallProcSetter::~uiFirewallProcSetter()
{
}


BufferStringSet uiFirewallProcSetter::getPythonExecList()
{
    BufferStringSet dirset;
    File::Path fp( getPythonInstDir() );
    fp.add( "envs" );
    DirList dirlist( fp.fullPath(), DirList::DLType::DirsInDir );
    if ( dirlist.isEmpty() )
	return dirset;

    for ( int idx=0; idx<dirlist.size(); idx++ )
    {
	File::Path exefp( dirlist.fullPath(idx) );
	BufferString foldernm = exefp.baseName();
	exefp.add( "python.exe" );

	if ( !File::exists(exefp.fullPath()) )
	    continue;

	dirset.add( foldernm );
    }
    return dirset;
}


BufferString uiFirewallProcSetter::getPythonInstDir()
{
    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    OD::PythonSource source;
    BufferString pythonloc;
    const bool pythonsource = OD::PythonSourceDef().parse( pythonsetts,
			    OD::PythonAccess::sKeyPythonSrc(), source );
    if ( !pythonsource || source != OD::PythonSource::Custom ||
	 !pythonsetts.get(OD::PythonAccess::sKeyEnviron(),pythonloc) )
	return ""; // return default location?

    return pythonloc;
}


bool uiFirewallProcSetter::acceptOK()
{
    BufferStringSet odchosenproc;
    odproclistbox_->getChosen( odchosenproc );

    BufferStringSet pythonchosenproc;
    pythonproclistbox_->getChosen( pythonchosenproc );

    if ( odchosenproc.isEmpty() && pythonchosenproc.isEmpty() )
    {
	uiMSG().error( uiStrings::phrPlsSelectAtLeastOne(uiStrings::sOption()));
	return false;
    }

    OS::MachineCommand cmd = "od_Setup_Firewall.exe";
    const bool toadd = addremfld_->getBoolValue();
    if ( toadd )
	cmd.addFlag( "add" );
    else
	cmd.addFlag( "remove" );

    bool errocc = false;
    for ( int idx=0; idx<2; idx++ ) //idx 0=od, idx1=python
    {
	OS::MachineCommand fincmd = cmd;
	if ( idx == 0 )
	    fincmd.addArg("o");
	else
	    fincmd.addArg("p");
	BufferStringSet procset = idx == 0 ? odchosenproc : pythonchosenproc;
	for ( int procidx=0; procidx<procset.size(); procidx++ )
	    fincmd.addArg( procset.get(procidx) );

	BufferString check = fincmd.toString();

	OS::CommandExecPars pars;
	pars.launchtype( OS::LaunchType::RunInBG );
	OS::MachineCommand mchcmd( fincmd.toString(&pars) );
	if ( !mchcmd.execute(pars) )
	{
	    uiString errmsg;
	    if ( toadd )
		errmsg = uiStrings::phrCannotAdd( tr("%1 process") );
	    else
		errmsg = uiStrings::phrCannotRemove( tr("%1 process") );

	    uiMSG().error( errmsg.arg( idx == 0 ? toUiString("OpendTect") :
							toUiString("Python")) );
	    errocc = true;
	}
    }

    if ( !errocc )
    {
	if ( toadd )
	    uiMSG().message( tr("Selected executables successfully added") );
	else
	    uiMSG().message( tr("Selected executables successfully removed") );
    }

    return true;
}
