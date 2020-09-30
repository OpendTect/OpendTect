/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/


#include "uisurveyselect.h"
#include "uidatarootsel.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uisurveymanager.h"

#include "dbman.h"
#include "file.h"
#include "filemonitor.h"
#include "filepath.h"
#include "survinfo.h"

uiSurveySelect::uiSurveySelect( uiParent* p, bool al )
    : uiSurveySelect(p,SurveyDiskLocation(),al)
{
}


uiSurveySelect::uiSurveySelect( uiParent* p, const SurveyDiskLocation& sdl,
				bool al )
    : uiGroup(p,"Survey selector")
    , dataroot_(sdl.basePath())
    , defsurvdirnm_(sdl.dirName())
    , filemonitor_(0)
    , topsep_(0)
    , survDirChg(this)
    , survParsChg(this)
    , survDirAccept(this)
{
    if ( defsurvdirnm_.isEmpty() )
	defsurvdirnm_ = SI().dirName();

    datarootfld_ = new uiDataRootSel( this, dataroot_ );
    if ( dataroot_.isEmpty() )
	dataroot_ = datarootfld_->getDir();

    maingrp_ = new uiGroup( this, "Main group" );
    survselgrp_ = new uiGroup( maingrp_, "Survey selection group" );

    uiListBox::Setup lbsu( OD::ChooseOnlyOne, uiStrings::sSurvey() );
    survdirfld_ = new uiListBox( survselgrp_, lbsu );
    survdirfld_->setHSzPol( uiObject::WideVar );
    survdirfld_->setStretch( 2, 2 );
    survselgrp_->setHAlignObj( survdirfld_ );
    maingrp_->setHAlignObj( survselgrp_ );

    if ( al )
    {
	maingrp_->attach( alignedBelow, datarootfld_ );
	setHAlignObj( datarootfld_ );
    }
    else
    {
	topsep_ = new uiSeparator( this, "Sep" );
	topsep_->attach( stretchedBelow, datarootfld_ );
	maingrp_->attach( ensureBelow, topsep_ );
    }

    mAttachCB( postFinalise(), uiSurveySelect::initGrp );
}


uiSurveySelect::~uiSurveySelect()
{
    stopFileMonitoring();
    detachAllNotifiers();
    deepErase( excludes_ );
}


bool uiSurveySelect::validSelection() const
{
    const BufferString drdir( datarootfld_->getDir() );
    return !drdir.isEmpty() && survdirfld_->currentItem() >= 0;
}


BufferString uiSurveySelect::getDirName() const
{
    return BufferString( survdirfld_->getText() );
}


BufferString uiSurveySelect::getFullDirPath() const
{
    return surveyDiskLocation().fullPath();
}


SurveyDiskLocation uiSurveySelect::surveyDiskLocation() const
{
    return SurveyDiskLocation( getDirName(), datarootfld_->getDir() );
}


void uiSurveySelect::setSurveyDiskLocation( const SurveyDiskLocation& sdl )
{
    datarootfld_->setDir( sdl.basePath() );
    defsurvdirnm_ = sdl.dirName();
    updateList();
    setSurveyDirName( sdl.dirName() );
}


void uiSurveySelect::setSurveyDirName( const char* survdirnm )
{
    defsurvdirnm_ = survdirnm;
    if ( finalised() && !defsurvdirnm_.isEmpty() )
	survdirfld_->setCurrentItem( defsurvdirnm_ );
}


void uiSurveySelect::addExclude( const SurveyDiskLocation& sdl )
{
    excludes_ += new SurveyDiskLocation( sdl );
    if ( finalised() )
	updateList();
}


void uiSurveySelect::updateList()
{
    stopFileMonitoring();

    NotifyStopper ns( survdirfld_->selectionChanged );

    BufferString prevsel( survdirfld_->getText() );
    if ( prevsel.isEmpty() )
	prevsel = defsurvdirnm_;

    survdirfld_->setEmpty();
    BufferStringSet dirlist;
    uiSurvey::getDirectoryNames( dirlist, false, dataroot_ );
    for ( int idx=0; idx<excludes_.size(); idx++ )
    {
	const SurveyDiskLocation& sdl = *excludes_[idx];
	if ( sdl.basePath() == dataroot_ )
	{
	    const int idxof = dirlist.indexOf( sdl.dirName() );
	    if ( idxof >= 0 )
		dirlist.removeSingle( idxof );
	}
    }
    survdirfld_->addItems( dirlist );

    if ( !survdirfld_->isEmpty() )
    {
	int newselidx = survdirfld_->indexOf( prevsel );
	if ( newselidx < 0 )
	    newselidx = survdirfld_->indexOf( defsurvdirnm_ );
	if ( newselidx < 0 )
	    newselidx = 0;
	survdirfld_->setCurrentItem( newselidx );
    }

    startFileMonitoring();
}


void uiSurveySelect::startFileMonitoring()
{
    stopFileMonitoring();
    filemonitor_ = new File::Monitor;
    filemonitor_->watch( dataroot_ );
    for ( int idx=0; idx<survdirfld_->size(); idx++ )
    {
	const File::Path fp( dataroot_, survdirfld_->itemText(idx),
					 SurveyInfo::sSetupFileName() );
	const BufferString fnm = fp.fullPath();
	filemonitor_->watch( fnm );
    }

    mAttachCB( filemonitor_->dirChanged, uiSurveySelect::dataRootChgCB );
    mAttachCB( filemonitor_->fileChanged, uiSurveySelect::survParFileChg );
}


void uiSurveySelect::stopFileMonitoring()
{
    delete filemonitor_;
    filemonitor_ = 0;
}


void uiSurveySelect::initGrp( CallBacker* )
{
    updateList();

    mAttachCB( datarootfld_->selectionChanged, uiSurveySelect::dataRootChgCB );
    mAttachCB( survdirfld_->selectionChanged, uiSurveySelect::survDirChgCB );
    mAttachCB( survdirfld_->doubleClicked, uiSurveySelect::survDirAcceptCB );
}


void uiSurveySelect::dataRootChgCB( CallBacker* cb )
{
    dataroot_ = datarootfld_->getDir();
    updateList();
    if ( !dataroot_.isEmpty() )
	survDirChg.trigger();
}


void uiSurveySelect::survDirChgCB( CallBacker* )
{
    survDirChg.trigger();
}


void uiSurveySelect::survDirAcceptCB( CallBacker* )
{
    survDirAccept.trigger();
}


void uiSurveySelect::survParFileChg( CallBacker* cb )
{
    mCBCapsuleUnpack( BufferString, fnm, cb );
    File::Path fp( fnm );
    fp.setFileName( 0 );
    const BufferString dirnm = fp.fileName();
    if ( dirnm == getDirName() )
	survParsChg.trigger();

    startFileMonitoring();
}



// uiSurvSel
uiSurvSel::uiSurvSel( uiParent* p, bool showmanager )
    : uiCompoundParSel(p,uiStrings::sSurvey())
    , showmanager_(showmanager)
{
    txtfld_->setStretch( 2, 2 );
    mAttachCB( butPush, uiSurvSel::doDlg );
}


uiSurvSel::~uiSurvSel()
{
    detachAllNotifiers();
}


void uiSurvSel::doDlg( CallBacker* )
{
    if ( showmanager_ )
    {
	uiSurveyManagerDlg dlg( this, false );
	dlg.go();
    }
    else
    {
	uiDialog dlg( this, uiDialog::Setup(
		toUiString("Select DataRoot and Survey"),
		mNoDlgTitle,mNoHelpKey));
	auto* survsel = new uiSurveySelect( &dlg );
	if ( !dlg.go() )
	    return;

	uiRetVal rv = DBM().setDataSource( survsel->getFullDirPath(), true );
	if ( !rv.isOK() )
	    uiMSG().error( rv );
    }
}


uiString uiSurvSel::getSummary() const
{
    return toUiString( SI().name() );
}
