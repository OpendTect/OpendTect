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


uiString getDlgTitle( uiFirewallProcSetter::ActionType acttyp )
{
    if ( acttyp == uiFirewallProcSetter::Add )
	return od_static_tr( "getDlgTitle", "Add Firewall Program Rule" );
    else if ( acttyp == uiFirewallProcSetter::Remove )
	return od_static_tr( "getDlgTitle", "Remove Firewall Program Rule" );
    else
	return od_static_tr( "getDlgTitle",
					"Add/Remove Firewall Program Rule" );
}

uiFirewallProcSetter::uiFirewallProcSetter( uiParent* p, ActionType acttyp,
						    const BufferString& path )
    : uiDialog(p, Setup(getDlgTitle(acttyp), mNoDlgTitle,
	mODHelpKey(mBatchHostsDlgHelpID)))
    , acttyp_(acttyp)
    , addremfld_(0)
    , pythonproclistbox_(0)
{
    if ( path.isEmpty() )
	exepath_ = GetExecPlfDir();
    else
	exepath_ = path;

    ePDD().getProcData( odprocnms_, odprocdescs_, ProcDesc::DataEntry::ODv6 );
    ePDD().getProcData( odprocnms_, odprocdescs_, ProcDesc::DataEntry::ODv7 );

    uiListBox::Setup su;
    su.lbl( tr("OpendTect Executables") );
    su.cm( OD::ChoiceMode::ChooseZeroOrMore );
    odproclistbox_ = new uiListBox( this, su );
    odproclistbox_->addItems( odprocdescs_ );
    odproclistbox_->setHSzPol( uiObject::SzPolicy::WideMax );
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

    const BufferStringSet& procnms = isodproc ? odprocnms_ : pyprocnms_;
    if ( isodproc )
	odproclistbox_->getChosen( selidxs );
    else if ( type == ProcDesc::DataEntry::Python && pythonproclistbox_ )
	pythonproclistbox_->getChosen( selidxs );

    for ( int idx=0; idx<selidxs.size(); idx++ )
	proclist.add( procnms.get(selidxs[idx]) );

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
