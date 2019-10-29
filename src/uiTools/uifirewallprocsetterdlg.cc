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

    ePDD().getProcData( odprocnms_, odprocdescs_, ProcDesc::DataEntry::OD );

    uiListBox::Setup su;
    su.lbl( tr("OpendTect Executables") );
    su.cm( OD::ChoiceMode::ChooseZeroOrMore );
    odproclistbox_ = new uiListBox( this, su );
    odproclistbox_->addItems( odprocdescs_ );
    odproclistbox_->setHSzPol( uiObject::SzPolicy::WideMax );
    uiObject* attachobj = odproclistbox_->attachObj();
    
    if ( !getPythonExecList().isEmpty() )
    {
	su.lbl( tr("Python Executables") );
	pythonproclistbox_ = new uiListBox( this, su );
	pythonproclistbox_->blockScrolling( false );
	pythonproclistbox_->addItems( getPythonExecList() );
	pythonproclistbox_->setHSzPol( uiObject::SzPolicy::WideMax );
	pythonproclistbox_->attach( alignedBelow, attachobj );
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
	return nullptr;

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
    if ( !pythonsource || source != OD::PythonSource::Custom ||
	!pythonsetts.get(OD::PythonAccess::sKeyEnviron(), pythonloc) )
	return "";

    return pythonloc;
}


BufferStringSet uiFirewallProcSetter::getSelProcList(
					    ProcDesc::DataEntry::Type type )
{
    BufferStringSet proclist;
    TypeSet<int> selidxs;

    const BufferStringSet& procnms = type == ProcDesc::DataEntry::OD ?
						    odprocnms_ : pyprocnms_;
    if ( type == ProcDesc::DataEntry::OD )
	odproclistbox_->getChosen( selidxs );
    else
	pythonproclistbox_->getChosen( selidxs );

    for ( int idx=0; idx<selidxs.size(); idx++ )
	proclist.add( procnms.get(selidxs[idx]) );

    return proclist;
}


bool uiFirewallProcSetter::acceptOK( CallBacker* )
{
    BufferStringSet odchosenproc = getSelProcList( ProcDesc::DataEntry::OD );

    BufferStringSet pythonchosenproc =
				getSelProcList( ProcDesc::DataEntry::Python );

    if ( odchosenproc.isEmpty() && pythonchosenproc.isEmpty() )
    {
	uiMSG().error( tr("Select atleast one option") );
	return false;
    }

    BufferString cmd = "od_Setup_Firewall.exe";

    bool toadd = acttyp_ == Add;
    if ( addremfld_ )
	toadd = addremfld_->getBoolValue();

    if ( toadd )
	cmd.add(" --add ");
    else
	cmd.add(" --remove ");
    
    bool errocc = false;
    const FilePath exepath( exepath_, "od_Setup_Firewall.exe" );
    for ( int idx=0; idx<2; idx++ ) //idx 0=od, idx1=python
    {
	BufferString fincmd = cmd;
	if ( idx == 0 )
	    fincmd.add( "--od " );
	else
	    fincmd.add( "--py " );

	const BufferStringSet& procset =
				    idx == 0 ? odchosenproc : pythonchosenproc;
	if ( procset.isEmpty() )
	    continue;

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
