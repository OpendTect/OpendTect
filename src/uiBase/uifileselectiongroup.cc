/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2017
 ________________________________________________________________________

-*/

#include "uifileselectiongroup.h"

#include "uilistbox.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uitoolbutton.h"
#include "uifileseltool.h"

#include "file.h"
#include "filepath.h"
#include "filesystemaccess.h"


uiFileSelectionGroup::uiFileSelectionGroup( uiParent* p, const Setup& su )
    : uiGroup(p,"File selection group")
    , setup_(su)
    , fsaselfld_(0)
    , newfolderbut_(0)
    , selChange(this)
{
    uiGroup* maingrp = createMainGroup();

    fnmfld_ = new uiLineEdit( this, FileNameInpSpec(), "File Name" );
    new uiLabel( this, uiStrings::sFileName( isSingle(setup_.selmode_)
			? 1 : mPlural ), fnmfld_ );
    setFileNames( setup_.initialselection_ );
    fnmfld_->attach( alignedBelow, maingrp );
    fnmfld_->setHSzPol( uiObject::WideVar );

    setHAlignObj( fnmfld_ );
}


void uiFileSelectionGroup::createFSAStuff( uiGroup* grp )
{
    File::SystemAccess::getProtocolNames( fsakeys_, !setup_.isForWrite() );
    const int nrfsakeys =
#ifdef ENABLE_REMOTE_FILESEL_UI
	fsakeys_.size();
#else
	1;
#endif
    if ( nrfsakeys < 2 )
	return;

    fsaselfld_ = new uiComboBox( grp, "Protocols" );
    for ( int idx=0; idx<fsakeys_.size(); idx++ )
    {
	const File::SystemAccess& fsa = File::SystemAccess::getByProtocol(
						fsakeys_.get(idx) );
	fsaselfld_->addItem( fsa.userName() );
	fsaselfld_->setIcon( idx, fsa.iconName() );
    }
}


uiGroup* uiFileSelectionGroup::createMainGroup()
{
    uiGroup* maingrp = new uiGroup( this, "Main group" );

    createFSAStuff( maingrp );

    filtfld_ = new uiComboBox( maingrp, "Filters" );
    File::FormatList fmlist( setup_.formats_ );
    if ( setup_.allowallextensions_ )
	fmlist.addFormat( File::Format::allFiles() );
    for ( int idx=0; idx<fmlist.size(); idx++ )
	filtfld_->addItem( fmlist.format(idx).userDesc() );
    if ( fsaselfld_ )
	filtfld_->attach( rightTo, fsaselfld_ );

    /* share with uiIOObjSel
    sortfld_ = new uiComboBox( maingrp, "Sorting" );
    sortfld_->addItem( tr("Alphabetical") );
    sortfld_->addItem( uiStrings::sTime() );
    */

    uiListBox::Setup lbsu;
    lbsu.prefnrlines( 8 ).prefwidth( 5 );
    uiGroup* leftgrp = 0;
    if ( setup_.isForWrite() || setup_.isForDirectory() )
	leftgrp = new uiGroup( maingrp, "Left group" );
    dirselfld_ = new uiListBox( leftgrp ? leftgrp : maingrp, lbsu,
				"Directories" );
    mAttachCB( dirselfld_->selectionChanged, uiFileSelectionGroup::dirSelCB );

    if ( !leftgrp )
	leftgrp = dirselfld_;
    else
    {
	newfolderbut_ = new uiToolButton( leftgrp, "newfolder",
			    tr("Create new folder"),
			    mCB(this,uiFileSelectionGroup,newFolderCrReqCB) );
	newfolderbut_->attach( ensureBelow, dirselfld_ );
	newfoldernamefld_ = new uiLineEdit( leftgrp, FileNameInpSpec(),
					    "New Folder Name" );
	newfoldernamefld_->attach( rightOf, newfolderbut_ );
    }

    lbsu.prefnrlines( 8 + (newfolderbut_ ? 1 : 0) ).prefwidth( 8 );
    lbsu.cm( isSingle(setup_.selmode_) ? OD::ChooseOnlyOne
				       : OD::ChooseZeroOrMore );
    leafselfld_ = new uiListBox( maingrp, lbsu, "Selectables" );
    leafselfld_->attach( rightOf, leftgrp );
    leafselfld_->attach( alignedBelow, filtfld_ );
    //sortfld_->attach( rightAlignedAbove, leafselfld_ );

    maingrp->setHAlignObj( leafselfld_ );
    return maingrp;
}


BufferString uiFileSelectionGroup::fileName() const
{
    BufferStringSet nms;
    getSelected( nms );
    return nms.isEmpty() ? BufferString() : nms.get(0);
}


void uiFileSelectionGroup::getSelected( BufferStringSet& nms ) const
{
    uiFileSelTool::separateSelection( fnmfld_->text(), nms );

    for ( int idx=0; idx<nms.size(); idx++ )
    {
	BufferString& fname = nms.get( idx );
	File::Path fp( fname );
	if ( !fp.isAbsolute() )
	{
	    //TODO insert current directory
	}
    }
}


const char* uiFileSelectionGroup::protocol() const
{
    return fsaselfld_ ? fsakeys_.get( fsaselfld_->currentItem() ).str()
		      : File::SystemAccess::getLocal().protocol();
}


void uiFileSelectionGroup::setFileName( const char* fnm )
{
    BufferStringSet fnms;
    if ( fnm && *fnm )
	fnms.add( fnm );
    setFileNames( fnms );
}


void uiFileSelectionGroup::setFileNames( const BufferStringSet& fnms )
{
    //TODO
}


void uiFileSelectionGroup::dirSelCB( CallBacker* )
{
}


void uiFileSelectionGroup::fnmSelCB( CallBacker* )
{
}


void uiFileSelectionGroup::filtChgCB( CallBacker* )
{
}


void uiFileSelectionGroup::sortChgCB( CallBacker* )
{
}


void uiFileSelectionGroup::fnmChgCB( CallBacker* )
{
}


void uiFileSelectionGroup::newFolderCrReqCB( CallBacker* )
{
}
