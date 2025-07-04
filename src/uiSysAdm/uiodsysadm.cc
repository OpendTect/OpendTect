/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodsysadm.h"
#include "uiodsysadmcoltabs.h"
#include "uishortcuts.h"
#include "uimain.h"
#include "uimsg.h"
#include "uilistbox.h"
#include "uilabel.h"
#include "uitextedit.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "plugins.h"
#include "oddirs.h"
#include "genc.h"
#include "file.h"
#include "moddepmgr.h"


static uiODSysAdm* mainODSysAdmMainWin( uiODSysAdm* i, bool set )
{
    mDefineStaticLocalObject( uiODSysAdm*, theinst, = nullptr );
    if ( set )
	theinst = i;
    return theinst;
}


uiODSysAdm* ODSysAdmMainWin()
{
    return mainODSysAdmMainWin( nullptr, false );
}


mGlobal(uiSysAdm) int ODSysAdmMain(uiMain&);

int ODSysAdmMain( uiMain& app )
{
    SetInSysAdmMode();

    OD::ModDeps().ensureLoaded( "uiTools" );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiSysAdm" );
    PtrMan<uiODSysAdm> topdlg = new uiODSysAdm( app );
    mainODSysAdmMainWin( topdlg.ptr(), true );
    app.setTopLevel( topdlg.ptr() );
    uiMSG().setMainWin( topdlg.ptr() );
    PIM().loadAuto( true );
    topdlg->show();

    return app.exec();
}

#define mAddGroup(nm) \
    groups_ += new GroupEntry( nm.getString() ); \
    grpfld->addItem( nm )
#define mAddTask(grpnr,nm,fn,comm) \
    te = new TaskEntry( nm, mCB(this,uiODSysAdm,fn), comm ); \
    groups_[grpnr]->tasks_ += te;


uiODSysAdm::uiODSysAdm( uiMain& a )
    : uiDialog(nullptr,Setup(tr("OpendTect System Administration"),mNoHelpKey))
	// The order of the following has to match the header file's!
    , swdir_(GetSoftwareDir(false))
    , asdir_(GetApplSetupDataDir())
    , haveas_(!asdir_.isEmpty() && File::exists(asdir_))
    , swwritable_(File::isWritable(swdir_))
    , aswritable_(haveas_ && File::isWritable(asdir_))
{

    if ( !swwritable_ && !aswritable_ )
    {
	uiString addonstr = haveas_ ? tr("\nnor to:%1").arg(asdir_)
				    : uiString::emptyString();
	uiString msg = tr("You have no write access to:\n%1%2")
		     .arg(swwritable_).arg(addonstr);
	new uiLabel( this, msg );
	setCtrlStyle( CloseOnly );
	return;
    }

    setCtrlStyle( RunAndClose );

    uiGroup* topgrp = new uiGroup( this, "Top group" );
    grpfld = new uiListBox( topgrp, "Task groups" );
    grpfld->selectionChanged.notify( mCB(this,uiODSysAdm,grpChg) );
    taskfld = new uiListBox( topgrp, "Tasks" );
    taskfld->selectionChanged.notify( mCB(this,uiODSysAdm,taskChg) );
    taskfld->doubleClicked.notify( mCB(this,uiODSysAdm,taskDClick) );
    taskfld->attach( rightOf, grpfld );
    topgrp->setHAlignObj( taskfld );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, topgrp, -2 );

    commentfld = new uiTextEdit( this, "Comments" );
    commentfld->attach( ensureBelow, sep );
    commentfld->setPrefHeightInChar( 2 );

    TaskEntry* te;
    mAddGroup( tr("Look & Feel") );
    mAddTask(0,"Color bars",doColorTabs,
	    "Manage standard color bars");
    mAddTask(0,"Shortcuts",doShortcuts,
	    "Manage standard keyboard shortcut settings");
    mAddTask(0,"Icon sets",doIconSets,"Add/Remove icons sets");
    mAddGroup( tr("Batch processing") );
    mAddTask(1,"Processing hosts",doBatchHosts,
	     "Manage hosts available for remote processing");
    mAddTask(1,"Batch programs",doBatchProgs,
	     "Manage batch programs available to users");
    mAddGroup( tr("FlexLM licensing") );
    mAddTask(2,"Install license file",doInstLicFile,
	     "Install a FlexLM license file used by plugin(s)");
    mAddTask(2,"Start license manager daemon",doStartLic,
	     "Start a FlexLM license manager daemon used by plugin(s)");
    mAddGroup( tr("Misc") );
    mAddTask(3,"Attribute sets",doAttribSets,
     "Add/Remove attribute sets to/from the standard default attribute sets" );

    preFinalize().notify( mCB(this,uiODSysAdm,setInitial) );
}


void uiODSysAdm::setInitial( CallBacker* )
{
    grpfld->setCurrentItem( 0 );
    taskfld->setCurrentItem( 0 );
    taskChg( this );
}


uiODSysAdm::~uiODSysAdm()
{
    deepErase( groups_ );
}


uiODSysAdm::GroupEntry* uiODSysAdm::getGroupEntry( const char* nm )
{
    for ( int idx=0; idx<groups_.size(); idx++ )
    {
	if ( groups_[idx]->name_ == nm )
	    return groups_[idx];
    }

    return nullptr;
}


uiODSysAdm::TaskEntry* uiODSysAdm::getTaskEntry( uiODSysAdm::GroupEntry* ge,
						 const char* nm )
{
    if ( ge )
    {
	for ( int idx=0; idx<ge->tasks_.size(); idx++ )
	{
	    if ( ge->tasks_[idx]->name_ == nm )
		return ge->tasks_[idx];
	}
    }
    return 0;
}


uiODSysAdm::TaskEntry* uiODSysAdm::getCurTaskEntry()
{
    return getTaskEntry( getGroupEntry(grpfld->getText()), taskfld->getText() );
}


void uiODSysAdm::grpChg( CallBacker* )
{
    taskfld->setEmpty();
    const GroupEntry* ge = getGroupEntry( grpfld->getText() );
    if ( !ge ) return;

    for ( int idx=0; idx<ge->tasks_.size(); idx++ )
	taskfld->addItem( ge->tasks_[idx]->name_ );

    taskfld->setCurrentItem( 0 );
    taskChg( this );
}


void uiODSysAdm::taskChg( CallBacker* )
{
    commentfld->setText( "" );
    uiODSysAdm::TaskEntry* te = getCurTaskEntry();
    if ( te )
	commentfld->setText( te->comment_ );
}


void uiODSysAdm::taskDClick( CallBacker* )
{
    taskChg( this );
    acceptOK( this );
}


bool uiODSysAdm::acceptOK( CallBacker* )
{
    if ( groups_.isEmpty() )
	return true;

    uiODSysAdm::TaskEntry* te = getCurTaskEntry();
    if ( te )
	te->cb_.doCall( this );

    return false;
}


void uiODSysAdm::doColorTabs( CallBacker* )
{
    uiODSysAdmColorTabs dlg( this );
    dlg.go();
}


void uiODSysAdm::doShortcuts( CallBacker* )
{
    uiShortcutsDlg dlg( this, "ODScene" );
    dlg.go();
}


void uiODSysAdm::doIconSets( CallBacker* )
{
    uiMSG().error( tr("Needs implementation") );
}


void uiODSysAdm::doBatchHosts( CallBacker* )
{
    uiMSG().error( tr("Needs implementation") );
}


void uiODSysAdm::doBatchProgs( CallBacker* )
{
    uiMSG().error( tr("Needs implementation") );
}


void uiODSysAdm::doInstLicFile( CallBacker* )
{
    uiMSG().error( tr("Needs implementation") );
}


void uiODSysAdm::doStartLic( CallBacker* )
{
    uiMSG().error( tr("Needs implementation") );
}


void uiODSysAdm::doAttribSets( CallBacker* )
{
    uiMSG().error( tr("Needs implementation") );
}
