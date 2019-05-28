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
    static BufferStringSet odprocnms;
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
    odproclistbox_->getChosen( pythonchosenproc );

    if ( odchosenproc.isEmpty() && pythonchosenproc.isEmpty() )
    {
	uiMSG().error( uiStrings::phrPlsSelectAtLeastOne(uiStrings::sOption()));
	return false;
    }

    BufferStringSet allprocs;
    allprocs.add( odchosenproc.getDispString() );
    allprocs.add( pythonchosenproc.getDispString() );

    OS::MachineCommand cmd = "od_Setup_Firewall.exe";
    if ( addremfld_->getBoolValue() )
	cmd.addFlag( "add" );
    else
	cmd.addFlag( "remove" );

    bool errocc = false;
    for ( int idx=0; idx<allprocs.size(); idx++ )
    {
	OS::MachineCommand fincmd = cmd;
	if ( idx == 0 )
	    fincmd.addArg("o");
	else
	    fincmd.addArg("p");
	fincmd.addArg( allprocs.get(idx) );

	OS::MachineCommand mchcmd( fincmd );
	OS::CommandExecPars pars;
	pars.launchtype( OS::LaunchType::RunInBG );
	if ( !mchcmd.execute(pars) )
	{
	    uiMSG().error( uiStrings::phrCannotAdd(tr("%1 process")
		   .arg(idx==0?toUiString("OpendTect"):toUiString("Python"))) );
	    errocc = true;
	}
    }

    if ( !errocc )
	uiMSG().message( tr("Selected executables successfully added") );

    return true;
}
