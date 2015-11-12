/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: $";


#include "uipresentationmaker.h"

#include "uibuttongroup.h"
#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uistring.h"
#include "uitable.h"
#include "uitoolbutton.h"

#include "file.h"
#include "filepath.h"
#include "od_ostream.h"
#include "oscommand.h"
#include "presentationspec.h"
#include "slidespec.h"

uiPresentationMakerDlg::uiPresentationMakerDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Presentation Maker"),mNoDlgTitle,mTODOHelpKey))
    , specs_(*new PresentationSpec)
{
    setModal( false );
    setCtrlStyle( CloseOnly );

    titlefld_ = new uiGenInput( this, tr("Presentation Title") );

    masterfld_ = new uiFileInput( this, tr("Master PPT") );
    masterfld_->attach( alignedBelow, titlefld_ );

    outputfld_ = new uiFileInput( this, tr("Output PPT") );
    outputfld_->attach( alignedBelow, masterfld_ );

    imagestorfld_ = new uiFileInput( this, tr("Image storage location") );
    imagestorfld_->attach( alignedBelow, outputfld_ );

    uiTable::Setup ts( 0, 2 );
    uiStringSet collbls;
    collbls.add( tr("Title") ).add( tr("Image") );
    slidestbl_ = new uiTable( this, ts, "Slides table" );
    slidestbl_->attach( alignedBelow, imagestorfld_ );

    uiButtonGroup* butgrp = new uiButtonGroup( this, "Buttons", OD::Vertical );
    new uiToolButton( butgrp, uiToolButton::UpArrow, uiStrings::sMoveUp(),
		      mCB(this,uiPresentationMakerDlg,moveUpCB) );
    new uiToolButton( butgrp, uiToolButton::DownArrow, uiStrings::sMoveDown(),
		      mCB(this,uiPresentationMakerDlg,moveDownCB) );
    new uiToolButton( butgrp, "remove", uiStrings::sRemove(),
		      mCB(this,uiPresentationMakerDlg,removeCB) );
    butgrp->attach( rightTo, slidestbl_ );

    uiPushButton* createbut = new uiPushButton( this, uiStrings::sCreate(),
	mCB(this,uiPresentationMakerDlg,createCB), true );
    createbut->attach( alignedBelow, slidestbl_ );
}


uiPresentationMakerDlg::~uiPresentationMakerDlg()
{
    delete &specs_;
}


bool uiPresentationMakerDlg::acceptOK( CallBacker* )
{
    return true;
}


void uiPresentationMakerDlg::addCB( CallBacker* )
{
}


void uiPresentationMakerDlg::moveUpCB( CallBacker* )
{
}


void uiPresentationMakerDlg::moveDownCB( CallBacker* )
{
}


void uiPresentationMakerDlg::removeCB( CallBacker* )
{
    const int currow = slidestbl_->currentRow();
    slidestbl_->removeRow( currow );
    specs_.removeSlide( currow );
}


void uiPresentationMakerDlg::createCB( CallBacker* )
{
    const FixedString title = titlefld_->text();
    if ( title.isEmpty() )
    {
	uiMSG().error( tr("Please provide a title") );
	return;
    }

    const FixedString outputfnm = outputfld_->fileName();
    if ( outputfnm.isEmpty() )
    {
	uiMSG().error( tr("Please provide output file name") );
	return;
    }

    specs_.setTitle( title );
    specs_.setOutputFilename( outputfnm );
    BufferString script;
    specs_.getPythonScript( script );
    BufferString scriptfnm = FilePath::getTempName("py");
    od_ostream strm( scriptfnm );
    strm << script.buf() << od_endl;

    BufferString cmd( "python ", scriptfnm );
    if ( !OS::ExecCommand( cmd.buf() ) )
    {
	uiMSG().error( tr("Could not execute\n: "), cmd.buf() );
	return;
    }

    const bool res = uiMSG().askGoOn(
	tr("Successfully created presentation. Open now?") );
    if ( !res ) return;

    uiDesktopServices::openUrl( outputfnm.buf() );
}

