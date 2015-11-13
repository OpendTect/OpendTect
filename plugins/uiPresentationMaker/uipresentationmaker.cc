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
#include "uicombobox.h"
#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "ui3dviewer.h"
#include "uiprintscenedlg.h"
#include "uiseparator.h"
#include "uistring.h"
#include "uitable.h"
#include "uitoolbutton.h"

#include "file.h"
#include "filepath.h"
#include "od_ostream.h"
#include "oscommand.h"
#include "presentationspec.h"
#include "slidespec.h"


class uiSlideLayoutDlg : public uiDialog
{ mODTextTranslationClass(uiSlideLayoutDlg)
public:
uiSlideLayoutDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Slide Layout"),mNoDlgTitle,mTODOHelpKey))
{
    uiLabel* lbl = new uiLabel( this, tr("Slide Format") );
    lbl->attach( leftBorder );
    widthfld_ = new uiGenInput( this, uiStrings::sWidth() );
    widthfld_->attach( ensureBelow, lbl );
    heightfld_ = new uiGenInput( this, uiStrings::sHeight() );
    heightfld_->attach( rightTo, widthfld_ );

    uiLabel* mlbl = new uiLabel( this, tr("Image Margins") );
    mlbl->attach( leftBorder );
    mlbl->attach( ensureBelow, widthfld_ );
    leftfld_ = new uiGenInput( this, tr("Left") );
    leftfld_->attach( alignedBelow, widthfld_ );
    leftfld_->attach( ensureBelow, mlbl );
    rightfld_ = new uiGenInput( this, tr("Right") );
    rightfld_->attach( rightTo, leftfld_ );
    topfld_ = new uiGenInput( this, tr("Top") );
    topfld_->attach( alignedBelow, leftfld_ );
    bottomfld_ = new uiGenInput( this, tr("Bottom") );
    bottomfld_->attach( rightTo, topfld_ );

}

protected:
    uiGenInput*		widthfld_;
    uiGenInput*		heightfld_;
    uiGenInput*		leftfld_;
    uiGenInput*		rightfld_;
    uiGenInput*		topfld_;
    uiGenInput*		bottomfld_;

};

uiPresentationMakerDlg::uiPresentationMakerDlg( uiParent* p )
    : uiDialog(0,Setup(tr("Presentation Maker"),mNoDlgTitle,mTODOHelpKey))
    , specs_(*new PresentationSpec)
{
    setModal( false );
    setCtrlStyle( CloseOnly );

    titlefld_ = new uiGenInput( this, tr("Presentation Title") );
    titlefld_->setElemSzPol( uiObject::Wide );

    uiFileInput::Setup fis;
    fis.forread(true);
    masterfld_ = new uiFileInput( this, tr("Master pptx"), fis );
    masterfld_->attach( alignedBelow, titlefld_ );

    fis.forread(false);
    outputfld_ = new uiFileInput( this, tr("Output pptx"), fis );
    outputfld_->attach( alignedBelow, masterfld_ );

    fis.forread(true).directories(true);
    imagestorfld_ = new uiFileInput( this, tr("Image Storage Location"), fis );
    imagestorfld_->attach( alignedBelow, outputfld_ );

    uiSeparator* sep = new uiSeparator( this, "HorSep", OD::Horizontal );
    sep->attach( stretchedBelow, imagestorfld_ );

    typegrp_ = new uiButtonGroup( this, "Type", OD::Horizontal );
    typegrp_->attach( alignedBelow, imagestorfld_ );
    typegrp_->attach( ensureBelow, sep );
    CallBack cb = mCB(this,uiPresentationMakerDlg,typeCB);
    new uiRadioButton( typegrp_, tr("Scene"), cb );
    new uiRadioButton( typegrp_, tr("Window"), cb );
    new uiRadioButton( typegrp_, tr("Desktop"), cb );
    typegrp_->selectButton( 0 );

    uiPushButton* addbut = new uiPushButton( this, tr("Add Slide"),
	mCB(this,uiPresentationMakerDlg,addCB), true );
    addbut->attach( rightTo, typegrp_ );

    windowfld_ = new uiComboBox( this, "Window Names" );
    windowfld_->setHSzPol( uiObject::Wide );
    windowfld_->attach( alignedBelow, typegrp_ );
    windowfld_->display( false );

    scenefld_ = new uiComboBox( this, "Scene Names" );
    scenefld_->setHSzPol( uiObject::Wide );
    scenefld_->attach( alignedBelow, typegrp_ );
    scenefld_->display( false );

    uiTable::Setup ts( 0, 1 );
    ts.rowdesc("Slide");
    slidestbl_ = new uiTable( this, ts, "Slides table" );
    slidestbl_->setColumnLabel( 0, tr("Title") );
    slidestbl_->attach( alignedBelow, windowfld_ );

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

    typeCB(0);
}


uiPresentationMakerDlg::~uiPresentationMakerDlg()
{
    delete &specs_;
}


bool uiPresentationMakerDlg::acceptOK( CallBacker* )
{
    return true;
}


void uiPresentationMakerDlg::updateWindowList()
{
    windowfld_->setEmpty();
    ObjectSet<uiMainWin> windowlist;
    getTopLevelWindows( windowlist );
    for ( int idx=0; idx<windowlist.size(); idx++ )
	windowfld_->addItem( windowlist[idx]->caption(true) );
}


void uiPresentationMakerDlg::updateSceneList()
{
    scenefld_->setEmpty();

    uiStringSet scenenames; int active = 0;
    ODMainWin()->sceneMgr().getSceneNames( scenenames, active );
    scenefld_->addItems( scenenames );
    scenefld_->setCurrentItem( active );
}


void uiPresentationMakerDlg::typeCB(CallBacker *)
{
    scenefld_->display( typegrp_->selectedId()==0 );
    windowfld_->display( typegrp_->selectedId()==1 );

    updateWindowList();
    updateSceneList();
}


static int slideidx = 1;
void uiPresentationMakerDlg::addCB( CallBacker* )
{
    FilePath imagefp( imagestorfld_->fileName() );
    imagefp.add( BufferString("image",slideidx) );
    imagefp.setExtension( "png" );
    const BufferString imagefnm = imagefp.fullPath();

    const bool grabscene = typegrp_->selectedId()==0;
    if ( grabscene )
    {
	const int selitm = scenefld_->currentItem();
	ObjectSet<ui3DViewer> vwrs;
	ODMainWin()->sceneMgr().get3DViewers( vwrs );
	if ( !vwrs.validIdx(selitm) )
	    return;

	ui3DViewer2Image vwr2image( *vwrs[selitm], imagefnm.buf() );
	vwr2image.create();
    }
    else
    {
	ObjectSet<uiMainWin> windowlist;
	getTopLevelWindows( windowlist );
	if ( windowlist.isEmpty() )
	    return;

	const int selitm = windowfld_->currentItem();
	const bool grabdesktop = typegrp_->selectedId()==2;
	const int zoom = grabdesktop ? 0 : 1;
	windowlist[selitm]->grab( imagefnm, zoom, "png" );
    }

    PresSlideSpec* ss = new PresSlideSpec;
    ss->imagefnm_ = imagefnm;
    ss->title_ = BufferString( "Slide ", slideidx );
    specs_.addSlide( *ss );
    slideidx++;

    const int row = slidestbl_->nrRows();
    slidestbl_->insertRows( row, 1 );
    slidestbl_->setText( RowCol(row,0), ss->title_ );
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

    const FixedString masterfnm = masterfld_->fileName();
    specs_.setMasterFilename( masterfnm.buf() );

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

