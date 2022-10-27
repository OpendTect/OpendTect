/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uifirewallprocsetterdlg.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistatusbar.h"

#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "oscommand.h"
#include "od_ostream.h"
#include "pythonaccess.h"
#include "settings.h"


uiString getWindowTitle()
{
    return od_static_tr( "getWindowTitle", "Manage OpendTect Firewall Rules" );
}

uiString getDialogTitle( ProcDesc::DataEntry::ActionType typ )
{
    if ( typ == ProcDesc::DataEntry::Add )
	return od_static_tr( "getDlgTitle",
	"Please allow the following apps to communicate through Firewall");
    else if ( typ == ProcDesc::DataEntry::Remove )
	return od_static_tr("getDlgTitle",
	"The following apps have been added to Firewall exception list.\n"
	"If you remove one, it might stop working properly.");
    else
	return mNoDlgTitle;
}



uiFirewallProcSetter::uiFirewallProcSetter( uiParent* p )
    : uiDialog(p, Setup(getWindowTitle(),mNoDlgTitle,
			mODHelpKey(mBatchHostsDlgHelpID)).nrstatusflds(-1))
    , addremfld_(nullptr)
    , pythonproclistbox_(nullptr)
    , odproclistbox_(nullptr)
{
    initUI();
}

uiFirewallProcSetter::uiFirewallProcSetter( uiParent* p,
			ProcDesc::DataEntry::ActionType acttyp,
			const BufferString* path, const BufferString* pypath )
    : uiDialog(p, Setup(getWindowTitle(),mNoDlgTitle,
			mODHelpKey(mBatchHostsDlgHelpID)).nrstatusflds(-1))
{
    initUI( path, pypath, acttyp );
}


#define mGetAddBool \
    if ( addremfld_ && addremfld_->isDisplayed() ) \
	toadd_ = addremfld_->getBoolValue(); \

#define mGetData \
    mGetAddBool \
    setEmpty(); \
    PDE::ActionType acttyp = toadd_ ? PDE::Add : PDE::Remove; \
    ePDD().getProcData( odprocnms_, odprocdescs_, PDE::OD, acttyp ); \
    pyprocdescs_ = getPythonExecList(); \


uiFirewallProcSetter::~uiFirewallProcSetter()
{
    detachAllNotifiers();
}


void uiFirewallProcSetter::initUI( const BufferString* path,
    const BufferString* pypath, PDE::ActionType acttyp )
{
    acttypstr_ = acttyp;
    setTitleText( getDialogTitle(acttyp) );
    if ( !path || path->isEmpty() )
	exepath_ = GetExecPlfDir();
    else
    {
	const FilePath fp( path->buf(), "bin", GetPlfSubDir(), GetBinSubDir() );
	exepath_ = fp.fullPath();
    }

    if ( !pypath || pypath->isEmpty() )
	pypath_ = getPythonInstDir();
    else
	pypath_ = *pypath;

    uiObject* attachobj( nullptr );
    uiListBox::Setup su;
    if ( acttyp == PDE::AddNRemove )
    {
	addremfld_ = new uiGenInput( this, uiStrings::sAction(),
	    BoolInpSpec(true, uiStrings::sAdd(), uiStrings::sRemove()) );
	mAttachCB( addremfld_->valuechanged,
				    uiFirewallProcSetter::selectionChgCB );
	attachobj = addremfld_->attachObj();
	toadd_ = addremfld_->getBoolValue();
    }
    else if ( acttyp == PDE::Remove )
    {
	setOkText( uiStrings::sRemove() );
	toadd_ = false;
    }
    else
    {
	setOkText( uiStrings::sAdd() );
	toadd_ = true;
    }

    init();

    su.lbl( tr("OpendTect Executables") );
    su.cm( OD::ChoiceMode::ChooseZeroOrMore );
    odproclistbox_ = new uiListBox( this, su );
    if ( addremfld_ || !odprocdescs_.isEmpty() )
    {
	if ( attachobj )
	    odproclistbox_->attach( alignedBelow, attachobj );

	odproclistbox_->addItems(odprocdescs_);
	odproclistbox_->setHSzPol(uiObject::SzPolicy::WideMax);
	mAttachCB(odproclistbox_->leftButtonClicked,
				uiFirewallProcSetter::statusUpdateODProcCB);
	mAttachCB(odproclistbox_->selectionChanged,
				uiFirewallProcSetter::statusUpdateODProcCB);
	odproclistbox_->chooseAll();
	attachobj = odproclistbox_->attachObj();
    }

    odproclistbox_->display( addremfld_ || !odprocdescs_.isEmpty() );

    su.lbl( tr("Python Executables") );
    pythonproclistbox_ = new uiListBox( this, su );
    pythonproclistbox_->blockScrolling( false );
    pythonproclistbox_->addItems( pyprocdescs_ );
    pythonproclistbox_->setHSzPol( uiObject::SzPolicy::WideMax );
    if ( addremfld_ || !pyprocdescs_.isEmpty() )
    {
	if ( attachobj )
	    pythonproclistbox_->attach( alignedBelow, attachobj );

	mAttachCB(pythonproclistbox_->leftButtonClicked,
			    uiFirewallProcSetter::statusUpdatePyProcCB);
	mAttachCB(pythonproclistbox_->selectionChanged,
				uiFirewallProcSetter::statusUpdatePyProcCB);
	pythonproclistbox_->chooseAll();
    }

    pythonproclistbox_->display( addremfld_|| !pyprocdescs_.isEmpty() );

    PDE::ActionType ty = ePDD().getActionType();
    if ( ty != PDE::AddNRemove )
    {
       setOkText( PDE::ActionTypeDef().getUiStringForIndex(ty) );
       if ( addremfld_ )
	   addremfld_->display( false );

       toadd_ = ty == PDE::Add;
       selectionChgCB( nullptr );
    }

    if ( addremfld_ )
	mAttachCB( addremfld_->valuechanged,
				    uiFirewallProcSetter::selectionChgCB );

    mAttachCB( postFinalize(), uiFirewallProcSetter::updateCB );
    mAttachCB( postFinalize(), uiFirewallProcSetter::updateAddRemFld );

}


void uiFirewallProcSetter::init()
{
    const FilePath fp( exepath_ );
    const BufferString exceptionlistpath = fp.dirUpTo( fp.nrLevels()-4 );
    ePDD().setPath( exceptionlistpath );
    mGetData
}


void uiFirewallProcSetter::updateUI( const BufferString& path,
			    const BufferString& pypath, PDE::ActionType typ )
{
    acttypstr_ = typ;
    setTitleText( getDialogTitle(typ) );
    if ( !path.isEmpty() )
    {
	const FilePath fp( path.buf(), "bin", GetPlfSubDir(),
							    GetBinSubDir() );
	exepath_ = fp.fullPath();
    }

    if ( !pypath.isEmpty() )
	pypath_ = pypath;

    init();

    updateAddRemFld( nullptr );
    selectionChgCB( nullptr );
    updateCB( nullptr );
}


void uiFirewallProcSetter::updateAddRemFld( CallBacker* )
{
    if ( !addremfld_ )
	return;

    PDE::ActionType ty = ePDD().getActionType();
    if ( ty != PDE::AddNRemove )
    {
	setOkText( PDE::ActionTypeDef().getUiStringForIndex(ty) );
	toadd_ = addremfld_->getBoolValue();
	addremfld_->display( false );
    }
}


void uiFirewallProcSetter::updateCB( CallBacker* )
{
    if ( odproclistbox_->isDisplayed() )
	statusUpdateODProcCB( nullptr );

    if ( pythonproclistbox_->isDisplayed() )
	statusUpdatePyProcCB( nullptr );
}


void uiFirewallProcSetter::setEmpty()
{
    pyprocnms_.setEmpty();
    pyprocdescs_.setEmpty();
    odprocnms_.setEmpty();
    odprocdescs_.setEmpty();
}


bool uiFirewallProcSetter::hasWorkToDo() const
{
    if ( odprocdescs_.isEmpty() && pyprocdescs_.isEmpty() )
	return false;

    const PDE::ActionType availacttype = ePDD().getActionType();
    if ( availacttype == PDE::AddNRemove || acttypstr_ == PDE::AddNRemove )
	return true;

    return acttypstr_ == availacttype;
}


void uiFirewallProcSetter::selectionChgCB( CallBacker* )
{
    odproclistbox_->setEmpty();
    pythonproclistbox_->setEmpty();

    mGetData
    odproclistbox_->addItems( odprocdescs_ );
    odproclistbox_->chooseAll();
    pythonproclistbox_->addItems( pyprocdescs_ );
    pythonproclistbox_->chooseAll();
}


void uiFirewallProcSetter::statusUpdateODProcCB( CallBacker* )
{
    int selidx = odproclistbox_->currentItem();
    if ( selidx < 0 )
	return;

    FilePath exefp( exepath_ );
    BufferString procnm;
    procnm = *odprocnms_[selidx];
    procnm.add( ".exe" );
    exefp.add( procnm );
    const BufferString procfnm = FilePath::getLongPath( exefp.fullPath() );
    statusBar()->message( tr("Path: %1").arg(procfnm), 0 );
}


void uiFirewallProcSetter::statusUpdatePyProcCB( CallBacker* )
{
    const int selidx = pythonproclistbox_->currentItem();
    if ( selidx < 0 )
	return;

    FilePath exefp( pypath_ );
    exefp.add( *pyprocnms_[selidx] );
    const BufferString procfnm = FilePath::getLongPath( exefp.fullPath() );
    statusBar()->message( tr("Path: %1").arg(procfnm), 0 );
}


uiStringSet uiFirewallProcSetter::getPythonExecList()
{
    PDE::ActionType acttyp = toadd_ ? PDE::Add : PDE::Remove;

    ePDD().getProcData( pyprocnms_, pyprocdescs_, PDE::Python, acttyp );

    if ( pyprocnms_.isEmpty() )
	return pyprocdescs_;

    FilePath fp( pypath_ );
    fp.add( "envs" );

    const char* pyexe = "python.exe";
    for ( int idx=pyprocnms_.size()-1; idx>=0; idx-- )
    {
	const FilePath pyexefp( fp.fullPath(), pyprocnms_.get(idx), pyexe );
	if ( !pyexefp.exists() )
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
    const IOPar& pythonsetts = Settings::fetch( "python" );
    OD::PythonSource source;
    BufferString pythonloc;
    const bool pythonsource = OD::PythonSourceDef().parse( pythonsetts,
	OD::PythonAccess::sKeyPythonSrc(), source );
    if ( !pythonsource )
	pythonloc.setEmpty();
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


BufferStringSet uiFirewallProcSetter::getProcList( PDE::Type type )
{
    BufferStringSet proclist;
    TypeSet<int> selidxs;

    const bool isodproc = type == PDE::OD;
    const BufferStringSet& procnms = isodproc ? odprocnms_ : pyprocnms_;
    if ( isodproc )
    {
	odproclistbox_->getChosen( selidxs );
    }
    else if ( type == PDE::Python && pythonproclistbox_ )
	pythonproclistbox_->getChosen( selidxs );

    for ( int idx=0; idx<selidxs.size(); idx++ )
    {
	const int selidx = selidxs[idx];
	proclist.add( procnms.get(selidx) );
    }

    return proclist;
}


bool uiFirewallProcSetter::acceptOK( CallBacker* )
{
    if ( !odproclistbox_->nrChosen() && !pythonproclistbox_->nrChosen() )
    {
	uiMSG().error( tr("Select at least one option") );
	return false;
    }

    const FilePath exepath( exepath_, "od_Setup_Firewall.exe" );

    bool errocc = false;
    IOPar pars;
    int nrprocsprocessed = 0;
    BufferStringSet failedprocnms;

    for ( int idx=0; idx<PDE::TypeDef().size(); idx++ )
    {
	const BufferStringSet& procset = getProcList(
			PDE::TypeDef().getEnumForIndex(idx) );
	if ( procset.isEmpty() )
	    continue;

	FilePath exefp( exepath_ );
	OS::MachineCommand mc( exepath.fullPath() );
	mc.addFlag( toadd_ ? "add" : "remove" );
	if ( idx != PDE::Python )
	    mc.addKeyedArg( "od", exefp.fullPath() );
	else
	    mc.addKeyedArg( "py", pypath_ );

	BufferStringSet procnmsset;
	for ( int procidx=0; procidx<procset.size(); procidx++ )
	{
	    mc.addArg( procset.get(procidx) );
	    procnmsset.add( procset.get(procidx) );
	}

	if ( !mc.execute() )
	{
	    failedprocnms.add( procnmsset,false );
	    errocc = true;
	}
	else
	{
	    pars.set( PDE::TypeDef().getKeyForIndex(idx), procnmsset );
	    nrprocsprocessed++;
	}
    }

    if ( !errocc )
    {
	if ( toadd_ )
	    uiMSG().message( tr("Selected apps successfully added") );
	else
	    uiMSG().message( tr("Selected apps successfully removed") );
    }
    else
    {
	uiString firstmsg = tr("Some modifications could not be made.\n"
		"Please make sure you run OpendTect as Administrator.");

	uiString errmsg = tr("\nThe following apps could not be %1:\n%2");
	errmsg.arg( toadd_ ? tr("added") : tr("removed") )
	      .arg( failedprocnms.getDispString() );

	uiMSG().errorWithDetails( errmsg, firstmsg );
	return true;
    }

    return  ePDD().writePars( pars, toadd_ );
}
