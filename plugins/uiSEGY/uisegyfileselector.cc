/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		August 2017
________________________________________________________________________

-*/

#include "uisegyfileselector.h"

#include "dirlist.h"
#include "file.h"
#include "filepath.h"

#include "uilistbox.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "uisegyimptype.h"
#include "uisegyreadstarter.h"

uiSEGYFileSelector::uiSEGYFileSelector(uiParent* p, const char* fnm,
				       const char* vntname,
				       const SEGY::ImpType& imptype )
    : uiDialog(p, uiDialog::Setup(tr("Select SEGY file(s)"),
	       mNoDlgTitle,mNoHelpKey) )
    , filenmsfld_(0)
    , imptype_( imptype )
{
    uiString titlestr;
    titlestr = tr("Please select the file(s) belonging to vintage '%1' " )
	      .arg( vntname );
    setTitleText( titlestr );
    File::Path fp( fnm );
    BufferString path = fp.pathOnly();
    dirnm_ = fp.pathOnly();
    BufferString msk("*.", fp.extension() );
    DirList filelist( path, File::FilesInDir, msk );
    filenmsfld_ = new uiListBox( this, "Select files", OD::ChooseAtLeastOne );
    filenmsfld_->addItems( filelist );
    filenmsfld_->setCurrentItem( fp.fileName() );
    filenmsfld_->setChosen( filenmsfld_->currentItem() );
    uiGroup* toolgrp = new uiGroup( this, "Tools group" );
    toolgrp->attach( centeredRightOf, filenmsfld_ );
    const CallBack scancb( mCB(this,uiSEGYFileSelector,quickScanCB) );
    uiToolButton* scanbut = new uiToolButton( toolgrp, "examine",
					      tr("Quick scan"), scancb );
    scanbut->setToolTip( tr("See the results of Quick Scan") );
    txtfld_ = new uiTextEdit( this, "Information", true );
    txtfld_->setPrefHeightInChar( 8 );
    txtfld_->setPrefWidthInChar( 80 );
    txtfld_->attach( alignedBelow, filenmsfld_ );
    filenmsfld_->selectionChanged.notify(
					mCB(this, uiSEGYFileSelector,selChgCB));
    selChgCB( 0 );
}


void uiSEGYFileSelector::selChgCB( CallBacker* )
{
    BufferString info;
    File::Path fp( dirnm_, filenmsfld_->getText() );
    info.add( "Location: " ).add( fp.fullPath() ).addNewLine();
    od_int64 szkb = File::getKbSize( fp.fullPath() );
    info.add( "Size: " ).add( szkb ).add( " KB" );
    info.addNewLine().add( "Last modified: ")
	.add( File::timeLastModified(fp.fullPath()) );
    txtfld_->setText( info );
}


void uiSEGYFileSelector::quickScanCB( CallBacker* )
{
    uiSEGYReadStarter::Setup su( false, &imptype_ );
    File::Path fp( dirnm_, filenmsfld_->getText() );
    const BufferString fnm ( fp.fullPath());
    su.filenm( fnm ).fixedfnm(true).vintagecheckmode(true);
    uiSEGYReadStarter* readstrdlg = new uiSEGYReadStarter( this, su);
    if ( !readstrdlg->go() )
	return;
}


void uiSEGYFileSelector::getSelNames( BufferStringSet& nms )
{
    filenmsfld_->getChosen( nms );
}


bool uiSEGYFileSelector::acceptOK()
{
    if ( !filenmsfld_->nrChosen() )
	return false;

    return true;
}
