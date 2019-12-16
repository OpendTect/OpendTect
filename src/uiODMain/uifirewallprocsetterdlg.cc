/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : May 2019
-*/

#include "uifirewallprocsetterdlg.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistatusbar.h"

#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "oddirs.h"
#include "oscommand.h"
#include "pythonaccess.h"
#include "settings.h"

mDefineEnumUtils(uiFirewallProcSetter,ActionType,"ActionType")
{
    "Add", "Remove", "AddNRemove", 0
};
 template <>
 void EnumDefImpl<uiFirewallProcSetter::ActionType>::init()
 {
     uistrings_ += uiStrings::sAdd();
     uistrings_ += uiStrings::sRemove();
     uistrings_ += tr("Not sure");
 }


uiString getWindowTitle( uiFirewallProcSetter::ActionType acttyp )
{
    if ( acttyp == uiFirewallProcSetter::Add )
	return od_static_tr( "getWindowTitle", "Add Firewall Program Rule" );
    else if ( acttyp == uiFirewallProcSetter::Remove )
	return od_static_tr( "getWindowTitle",
					    "Remove Firewall Program Rule" );
    else
	return od_static_tr( "getWindowTitle",
					"Add/Remove Firewall Program Rule" );
}

uiString getDlgTitle()
{
    return od_static_tr( "getDlgTitle",
	"Please add following rule before launching OpendTect for smooth "
	"running of program.");
}

uiFirewallProcSetter::uiFirewallProcSetter( uiParent* p, ActionType acttyp,
						    const BufferString& path )
    : uiDialog(p, Setup(getWindowTitle(acttyp), getDlgTitle(),
			    mODHelpKey(mBatchHostsDlgHelpID)).nrstatusflds(-1))
    , acttyp_(acttyp)
    , addremfld_(0)
    , pythonproclistbox_(0)
{
    if ( path.isEmpty() )
	exepath_ = GetExecPlfDir();
    else
	exepath_ = path;

    ePDD().getProcData( odv6procnms_, odprocdescs_, ProcDesc::DataEntry::ODv6 );
    ePDD().getProcData( odv7procnms_, odprocdescs_, ProcDesc::DataEntry::ODv7 );

    uiListBox::Setup su;
    su.lbl( tr("OpendTect Executables") );
    su.cm( OD::ChoiceMode::ChooseZeroOrMore );
    odproclistbox_ = new uiListBox( this, su );
    odproclistbox_->addItems( odprocdescs_ );
    odproclistbox_->setHSzPol( uiObject::SzPolicy::WideMax );
    mAttachCB(odproclistbox_->selectionChanged,
				uiFirewallProcSetter::statusUpdateODProcCB);
    odproclistbox_->chooseAll();
    uiObject* attachobj = odproclistbox_->attachObj();
    
    if ( !getPythonExecList().isEmpty() )
    {
	su.lbl( tr("Python Executables") );
	pythonproclistbox_ = new uiListBox( this, su );
	pythonproclistbox_->blockScrolling( false );
	pythonproclistbox_->addItems( getPythonExecList() );
	pythonproclistbox_->setHSzPol( uiObject::SzPolicy::WideMax );
	pythonproclistbox_->attach( alignedBelow, attachobj );
	mAttachCB(pythonproclistbox_->selectionChanged,
				uiFirewallProcSetter::statusUpdatePyProcCB);
	pythonproclistbox_->chooseAll();
	attachobj = pythonproclistbox_->attachObj();
    }
    if ( acttyp == AddNRemove )
    {
	addremfld_ = new uiGenInput( this, uiStrings::sAction(), 
	    BoolInpSpec(true, uiStrings::sAdd(), uiStrings::sRemove()) );
	addremfld_->attach( alignedBelow, attachobj );
    }
    else
	setOkText( acttyp == Add ? uiStrings::sAdd() : uiStrings::sRemove() );
}


uiFirewallProcSetter::~uiFirewallProcSetter()
{
}


void uiFirewallProcSetter::statusUpdateODProcCB( CallBacker* cb )
{
    int selidx = odproclistbox_->currentItem();
    const int v6procsz = odv6procnms_.size();
    BufferString procfp;
    FilePath exefp( exepath_ );
    const BufferString str = exefp.fullPath();
    BufferString procnm;
    if ( selidx < v6procsz )
    {
	procnm = *odv6procnms_[selidx];
	procnm.add( ".exe" );
	exefp.add( procnm );
	procfp = exefp.fullPath();
    }
    else
    {
	selidx -= v6procsz;

	FilePath odv7fp( exefp.dirUpTo(exefp.nrLevels()-4) );
	odv7fp.add( "v7" ).add( "bin" ).add( GetPlfSubDir() )
						    .add( GetBinSubDir() );
	procnm = *odv7procnms_[selidx];
	procnm.add( ".exe" );
	odv7fp.add( procnm );
	procfp = odv7fp.fullPath();
    }

    uiString msg = tr("Selected process path : %1").arg( procfp );
    statusBar()->message( msg, 0 );
}


void uiFirewallProcSetter::statusUpdatePyProcCB(CallBacker* cb)
{
    const int selidx = pythonproclistbox_->currentItem();
    FilePath exefp( getPythonInstDir() );
    exefp.add( *pyprocnms_[selidx] );
    const BufferString procfp = exefp.fullPath();
    uiString msg = tr("Selected process path : %1").arg( procfp );
    statusBar()->message( msg, 0 );
}


uiStringSet uiFirewallProcSetter::getPythonExecList()
{
    pyprocnms_.setEmpty();
    pyprocdescs_.setEmpty();

    ePDD().getProcData( pyprocnms_, pyprocdescs_,
						ProcDesc::DataEntry::Python );

    if ( pyprocnms_.isEmpty() )
	return pyprocdescs_;

    FilePath fp( getPythonInstDir() );
    fp.add( "envs" );

    for ( int idx=pyprocnms_.size()-1; idx>=0; idx-- )
    {
	const FilePath pyexefp( fp.fullPath(), pyprocnms_.get(idx),
								"python.exe" );

	if ( !File::exists(pyexefp.fullPath()) )
	{
	    pyprocnms_.removeSingle( idx );
	    pyprocdescs_.removeSingle( idx );
	    continue;
	}
    }

    return pyprocdescs_;
}


BufferString uiFirewallProcSetter::getPythonInstDir()
{
    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    OD::PythonSource source;
    BufferString pythonloc;
    const bool pythonsource = OD::PythonSourceDef().parse( pythonsetts,
	OD::PythonAccess::sKeyPythonSrc(), source );
    if ( !pythonsource )
	pythonloc = "";
    else if ( source == OD::PythonSource::Custom )
	pythonsetts.get( OD::PythonAccess::sKeyEnviron(), pythonloc );
    else
    {
	FilePath fp;
	OD::PythonAccess::getPathToInternalEnv( fp, true );
	pythonloc = fp.fullPath();
    }

    return pythonloc;
}


BufferStringSet uiFirewallProcSetter::getSelProcList(
					    ProcDesc::DataEntry::Type type )
{
    BufferStringSet proclist;
    TypeSet<int> selidxs;

    const bool isodproc = type == ProcDesc::DataEntry::ODv6 ||
				type == ProcDesc::DataEntry::ODv7;

    const BufferStringSet& procnms =
	isodproc && type == ProcDesc::DataEntry::ODv6 ? odv6procnms_ :
	isodproc && type == ProcDesc::DataEntry::ODv7 ? odv7procnms_ :
								pyprocnms_;
    if ( isodproc )
    {
	odproclistbox_->getChosen( selidxs );
    }
    else if ( type == ProcDesc::DataEntry::Python && pythonproclistbox_ )
	pythonproclistbox_->getChosen( selidxs );

    for ( int idx=0; idx<selidxs.size(); idx++ )
    {
	int selidx = selidxs[idx];
	const int v6procsz = odv6procnms_.size();
	if ( type == ProcDesc::DataEntry::ODv6 && selidx >= v6procsz )
	    continue;
	else if ( type == ProcDesc::DataEntry::ODv7 )
	{
	    selidx -= v6procsz;
	    if ( selidx < 0 )
		continue;
	}

	proclist.add( procnms.get(selidx) );
    }

    return proclist;
}


bool uiFirewallProcSetter::acceptOK( CallBacker* )
{
    if ( !odproclistbox_->nrChosen() && !pythonproclistbox_->nrChosen() )
    {
	uiMSG().error( tr("Select atleast one option") );
	return false;
    }

    const FilePath exepath( exepath_, "od_Setup_Firewall.exe" );
    BufferString cmd;

    bool toadd = acttyp_ == Add;
    if ( addremfld_ )
	toadd = addremfld_->getBoolValue();

    if ( toadd )
	cmd.add(" --add ");
    else
	cmd.add(" --remove ");
    
    bool errocc = false;
    for ( int idx=0; idx<ProcDesc::DataEntry::TypeDef().size(); idx++ )
    {
	const BufferStringSet& procset = getSelProcList(
			ProcDesc::DataEntry::TypeDef().getEnumForIndex(idx) );
	if ( procset.isEmpty() )
	    continue;

	if ( idx == ProcDesc::DataEntry::ODv7 )
	{
	    const FilePath exefp( exepath_ );
	    FilePath odv7fp( exefp.dirUpTo(exefp.nrLevels()-4) );
	    odv7fp.add( "v7" ).add( "bin" ).add( GetPlfSubDir() )
							.add( GetBinSubDir() );
	    exepath_ = odv7fp.fullPath();
	}

	BufferString fincmd = cmd;
	if ( idx == 0 )
	    fincmd.add( "--od " ).add( exepath_ ).addSpace();
	else
	    fincmd.add( "--py " ).add( getPythonInstDir() ).addSpace();

	for ( int procidx=0; procidx<procset.size(); procidx++ )
	    fincmd.add( procset.get(procidx) ).addSpace();

	if ( !ExecODProgram(exepath.fullPath(),fincmd) )
	{
	    uiString errmsg;
	    if ( toadd )
		errmsg = uiStrings::phrCannotAdd( tr("%1 process") );
	    else
		errmsg = uiStrings::phrCannotRemove( tr("%1 process") );

	    uiMSG().errorWithDetails(
		    errmsg.arg(idx == 0 ? ::toUiString("OpendTect") :
		    ::toUiString("Python")),
		    tr("Please make sure OpendTect is running "
					"in administrative mode") );
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
