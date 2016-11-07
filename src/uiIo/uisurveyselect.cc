/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/


#include "uisurveyselect.h"
#include "uidatarootsel.h"
#include "uilistbox.h"
#include "uiseparator.h"
#include "uimsg.h"

#include "file.h"
#include "filepath.h"
#include "filemonitor.h"
#include "survinfo.h"
#include "oddirs.h"



uiSurveySelect::uiSurveySelect( uiParent* p, bool al, const char* dr,
				const char* survdirnm )
    : uiGroup(p,"Survey selector")
    , dataroot_(dr ? dr : GetBaseDataDir())
    , filemonitor_(0)
    , survDirChg(this)
    , survParsChg(this)
    , survDirAccept(this)
{
    datarootfld_ = new uiDataRootSel( this, dataroot_ );

    uiListBox::Setup lbsu( OD::ChooseOnlyOne, uiStrings::sSurvey() );
    survdirfld_ = new uiListBox( this, lbsu );
    updateList();
    BufferString defsurvdirnm( survdirnm );
    if ( defsurvdirnm.isEmpty() )
	defsurvdirnm = SI().getDirName();
    setSurveyDirName( defsurvdirnm );
    survdirfld_->setHSzPol( uiObject::WideVar );
    survdirfld_->setStretch( 2, 2 );

    if ( al )
    {
	survdirfld_->attach( alignedBelow, datarootfld_ );
	setHAlignObj( datarootfld_ );
    }
    else
    {
	uiSeparator* sep = new uiSeparator( this, "Sep" );
	sep->attach( stretchedBelow, datarootfld_ );
	survdirfld_->attach( ensureBelow, sep );
    }

    mAttachCB( datarootfld_->selectionChanged, uiSurveySelect::dataRootChgCB );
    mAttachCB( survdirfld_->selectionChanged, uiSurveySelect::survDirChgCB );
    mAttachCB( survdirfld_->doubleClicked, uiSurveySelect::survDirAcceptCB );
}


uiSurveySelect::~uiSurveySelect()
{
    stopFileMonitoring();
    detachAllNotifiers();
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
    const File::Path fp( datarootfld_->getDir(), getDirName() );
    return fp.fullPath();
}


void uiSurveySelect::setSurveyDirName( const char* survdirnm )
{
    if ( survdirnm && *survdirnm )
	survdirfld_->setCurrentItem( survdirnm );
}


void uiSurveySelect::updateList()
{
    stopFileMonitoring();
    NotifyStopper ns( survdirfld_->selectionChanged );

    const BufferString prevsel( survdirfld_->getText() );
    survdirfld_->setEmpty();
    BufferStringSet dirlist; uiSurvey::getDirectoryNames( dirlist, false,
							  dataroot_ );
    survdirfld_->addItems( dirlist );

    if ( !survdirfld_->isEmpty() )
    {
	int newselidx = survdirfld_->indexOf( prevsel );
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
	const File::Path fp( dataroot_, survdirfld_->textOfItem(idx),
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


void uiSurveySelect::dataRootChgCB( CallBacker* cb )
{
    dataroot_ = datarootfld_->getDir();
    updateList();
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
