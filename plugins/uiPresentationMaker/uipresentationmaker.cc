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
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiodattribtreeitem.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uiprintscenedlg.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uistring.h"
#include "uitable.h"
#include "uitoolbutton.h"
#include "uivispartserv.h"
#include "ui3dviewer.h"

#include "attribsel.h"
#include "file.h"
#include "filepath.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "od_ostream.h"
#include "oscommand.h"


class uiSlideLayoutDlg : public uiDialog
{ mODTextTranslationClass(uiSlideLayoutDlg)
public:
uiSlideLayoutDlg( uiParent* p, PresentationSpec& spec )
    : uiDialog(p,Setup(tr("Settings"),mNoDlgTitle,
		       mODHelpKey(mSlideLayoutDlgHelpID)))
    , spec_(spec)
{
    uiLabel* lbl = new uiLabel( this, tr("Slide Format:") );
    lbl->attach( leftBorder );

    uiLabeledComboBox* lcc = new uiLabeledComboBox( this, tr("Format") );
    formatfld_ = lcc->box();
    formatfld_->setHSzPol( uiObject::MedVar );
    lcc->attach( ensureBelow, lbl );

    uiStringSet formats;
    formats.add( tr("Screen 4:3") )
	   .add( tr("Screen 16:9 (2007-2010)") )
	   .add( tr("Screen 16:9 (2013)") )
	   .add( tr("User Defined") );
    formatfld_->addItems( formats );
    formatfld_->selectionChanged.notify( mCB(this,uiSlideLayoutDlg,formatCB) );

    uiLabeledSpinBox* wf = new uiLabeledSpinBox( this, uiStrings::sWidth(), 2 );
    widthfld_ = wf->box();
    uiLabeledSpinBox* hf = new uiLabeledSpinBox( this, uiStrings::sHeight(), 2);
    heightfld_ = hf->box();

    wf->attach( alignedBelow, lcc );
    hf->attach( rightTo, wf );

    uiSeparator* sep1 = new uiSeparator( this, "Sep1" );
    sep1->attach( stretchedBelow, wf );

    uiLabel* ilbl = new uiLabel( this, tr("Image Margins:") );
    ilbl->attach( leftBorder );
    ilbl->attach( ensureBelow, sep1 );

    uiLabeledSpinBox* lf = new uiLabeledSpinBox( this, tr("Left"), 2 );
    leftfld_ = lf->box();
    uiLabeledSpinBox* rf = new uiLabeledSpinBox( this, tr("Right"), 2 );
    rightfld_ = rf->box();
    uiLabeledSpinBox* tf = new uiLabeledSpinBox( this, tr("Top"), 2 );
    topfld_ = tf->box();
    uiLabeledSpinBox* bf = new uiLabeledSpinBox( this, tr("Bottom"), 2 );
    bottomfld_ = bf->box();

    lf->attach( alignedBelow, wf );
    lf->attach( ensureBelow, ilbl );
    rf->attach( rightTo, lf );
    rf->attach( alignedBelow, hf );
    tf->attach( alignedBelow, lf );
    bf->attach( rightTo, tf );
    bf->attach( alignedBelow, rf );

    SlideLayout& layout = spec_.getSlideLayout();
    formatfld_->setCurrentItem( layout.format_ );
    formatCB(0);
    if ( layout.format_==3 )
    {
	widthfld_->setValue( layout.width_ );
	heightfld_->setValue( layout.height_ );
    }

    leftfld_->setValue( layout.left_ );
    rightfld_->setValue( layout.right_ );
    topfld_->setValue( layout.top_ );
    bottomfld_->setValue( layout.bottom_ );
}


protected:
void formatCB( CallBacker* )
{
    const int sel = formatfld_->currentItem();
    if ( sel==0 )
    { widthfld_->setValue( 10 ); heightfld_->setValue( 7.5 ); }
    if ( sel==1 )
    { widthfld_->setValue( 10 ); heightfld_->setValue( 5.63 ); }
    if ( sel==2 )
    { widthfld_->setValue( 13.33 ); heightfld_->setValue( 7.5 ); }

    widthfld_->setSensitive( sel==3 );
    heightfld_->setSensitive( sel==3 );
}


bool acceptOK( CallBacker* )
{
    SlideLayout& layout = spec_.getSlideLayout();
    layout.format_ = formatfld_->currentItem();
    layout.width_ = widthfld_->getFValue();
    layout.height_ = heightfld_->getFValue();
    layout.left_ = leftfld_->getFValue();
    layout.right_ = rightfld_->getFValue();
    layout.top_ = topfld_->getFValue();
    layout.bottom_ = bottomfld_->getFValue();

    layout.saveToSettings();
    return true;
}

    PresentationSpec&	spec_;
    uiComboBox*		formatfld_;
    uiSpinBox*		widthfld_;
    uiSpinBox*		heightfld_;
    uiSpinBox*		leftfld_;
    uiSpinBox*		rightfld_;
    uiSpinBox*		topfld_;
    uiSpinBox*		bottomfld_;
};


uiPresentationMakerDlg::uiPresentationMakerDlg( uiParent* )
    : uiDialog(0, Setup(tr("Presentation Maker"),mNoDlgTitle,
			mODHelpKey(mPresentationMakerDlgHelpID)))
{
    setModal( false );
    setCtrlStyle( CloseOnly );

    titlefld_ = new uiGenInput( this, tr("Presentation Title") );
    titlefld_->setElemSzPol( uiObject::Wide );

    const BufferString templfnm = PresentationSpec::getTemplate();
    const bool isblank = templfnm.isEmpty();
    templatefld_ = new uiGenInput( this, tr("Template"),
	BoolInpSpec(isblank,tr("Blank"),tr("Custom")) );
    templatefld_->valuechanged.notify(
		mCB(this,uiPresentationMakerDlg,templateCB) );
    templatefld_->attach( alignedBelow, titlefld_ );

    settingsbut_ =
	new uiToolButton( this, "settings", tr("Slide Layout"),
			  mCB(this,uiPresentationMakerDlg,settingsCB) );
    settingsbut_->attach( rightTo, templatefld_ );

    BufferString filter( "PowerPoint (*.pptx)" );
    uiFileInput::Setup fis;
    fis.forread(true).filter( filter );
    masterfld_ = new uiFileInput( this, tr("Template pptx"), fis );
    masterfld_->setFileName( templfnm );
    masterfld_->attach( alignedBelow, templatefld_ );

    fis.forread(false);
    outputfld_ = new uiFileInput( this, tr("Output pptx"), fis );
    outputfld_->attach( alignedBelow, masterfld_ );

    uiSeparator* sep = new uiSeparator( this, "HorSep", OD::Horizontal );
    sep->attach( stretchedBelow, outputfld_ );

    typegrp_ = new uiButtonGroup( this, "Type", OD::Horizontal );
    typegrp_->attach( alignedBelow, outputfld_ );
    typegrp_->attach( ensureBelow, sep );
    CallBack cb = mCB(this,uiPresentationMakerDlg,imageTypeCB);
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
    slidestbl_->setPrefHeightInChar( 4 );
    slidestbl_->attach( ensureBelow, windowfld_ );

    uiButtonGroup* butgrp = new uiButtonGroup( this, "Buttons", OD::Vertical );
    new uiToolButton( butgrp, uiToolButton::UpArrow, uiStrings::sMoveUp(),
		      mCB(this,uiPresentationMakerDlg,moveUpCB) );
    new uiToolButton( butgrp, uiToolButton::DownArrow, uiStrings::sMoveDown(),
		      mCB(this,uiPresentationMakerDlg,moveDownCB) );
    new uiToolButton( butgrp, "remove", uiStrings::sRemove(),
		      mCB(this,uiPresentationMakerDlg,removeCB) );
    butgrp->attach( rightTo, slidestbl_ );

    uiPushButton* createbut = new uiPushButton( this, tr("Create Presentation"),
	mCB(this,uiPresentationMakerDlg,createCB), true );
    createbut->attach( centeredBelow, slidestbl_ );

    templateCB(0);
    imageTypeCB(0);
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


void uiPresentationMakerDlg::templateCB( CallBacker* )
{
    const bool isblank = templatefld_->getBoolValue();
    masterfld_->display( !isblank );
    settingsbut_->display( !isblank );
}


void uiPresentationMakerDlg::imageTypeCB( CallBacker* )
{
    scenefld_->display( typegrp_->selectedId()==0 );
    windowfld_->display( typegrp_->selectedId()==1 );

    updateWindowList();
    updateSceneList();
}


void uiPresentationMakerDlg::settingsCB( CallBacker* )
{
    uiSlideLayoutDlg dlg( this, specs_ );
    dlg.go();
}


static void createSlideName( BufferString& slidename )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const int visid = visserv->getSelObjectId();
    visserv->getObjectInfo( visid, slidename );
    if ( slidename.isEmpty() )
    {
	uiString objnm = visserv->getObjectName( visid );
	slidename = objnm.getFullString();
    }

    const int attrib = visserv->getSelAttribNr();
    if ( attrib >= 0 )
    {
	uiString dispnm =
		uiODAttribTreeItem::createDisplayName( visid, attrib );
	BufferString attribnm = dispnm.getFullString();
	if ( attribnm.isEmpty() )
	{
	    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
	    attribnm = as ? as->userRef() : "";
	}

	if ( !attribnm.isEmpty() )
	    slidename.add( " - " ).add( attribnm.buf() );
    }
}


static int slideidx = 1;
static const char* datefmt = "yyyyMMddhhsszzz";

void uiPresentationMakerDlg::addCB( CallBacker* )
{
    MouseCursorChanger mcc( MouseCursor::Wait );

    FilePath imagefp( PresentationSpec::getPyDir() );
    imagefp.add( BufferString("image-",Time::getDateTimeString(datefmt)) );
    imagefp.setExtension( "png" );
    const BufferString imagefnm = imagefp.fullPath();

    BufferString slidename;
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

	createSlideName( slidename );
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
	slidename = grabdesktop ? "Desktop"
		: windowlist[selitm]->caption(true).getFullString();
    }

    SlideContent* ss = new SlideContent( slidename, imagefnm );
    specs_.addSlide( *ss );
    slideidx++;

    const int row = slidestbl_->nrRows();
    slidestbl_->insertRows( row, 1 );
    slidestbl_->setText( RowCol(row,0), slidename );
}


void uiPresentationMakerDlg::moveUpCB( CallBacker* )
{
    const int currow = slidestbl_->currentRow();
    if ( currow < 1 ) return;

    specs_.swapSlides( currow, currow-1 );
    const RowCol from = RowCol(currow,0);
    const RowCol to = RowCol(currow-1,0);
    const BufferString txt0 = slidestbl_->text( from );
    const BufferString txt1 = slidestbl_->text( to );
    slidestbl_->setText( from, txt1 );
    slidestbl_->setText( to, txt0 );
    slidestbl_->setCurrentCell( to );
}


void uiPresentationMakerDlg::moveDownCB( CallBacker* )
{
    const int currow = slidestbl_->currentRow();
    if ( currow > slidestbl_->nrRows() -1 ) return;

    specs_.swapSlides( currow, currow+1 );
    const RowCol from = RowCol(currow,0);
    const RowCol to = RowCol(currow+1,0);
    const BufferString txt0 = slidestbl_->text( from );
    const BufferString txt1 = slidestbl_->text( to );
    slidestbl_->setText( from, txt1 );
    slidestbl_->setText( to, txt0 );
    slidestbl_->setCurrentCell( to );
}


void uiPresentationMakerDlg::removeCB( CallBacker* )
{
    const int currow = slidestbl_->currentRow();
    if ( currow < 0 || currow >= specs_.nrSlides() )
	return;

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

    const BufferString outputfnm = outputfld_->fileName();
    if ( outputfnm.isEmpty() )
    {
	uiMSG().error( tr("Please provide output file name") );
	return;
    }

    if ( File::exists(outputfnm) )
    {
	const bool res = File::remove( outputfnm );
	if ( !res )
	{
	    uiMSG().error( tr("Output file exists but cannot be removed."
		"File is possibly in use. Please close before overwriting.") );
	    return;
	}
    }

    specs_.setTitle( title );
    specs_.setOutputFilename( outputfnm );

    const bool isblankpres = templatefld_->getBoolValue();
    const BufferString templfnm = !isblankpres ? masterfld_->fileName() : "";
    // TODO: merge 2 calls below
    specs_.setMasterFilename( templfnm.buf() );
    PresentationSpec::setTemplate( templfnm.buf() );
    if ( !isblankpres && !File::exists(templfnm.buf()) )
    {
	uiMSG().error( tr("Template pptx does not exist. "
	    "Select another template pptx or create a blank presentation.") );
	return;
    }

    const int nrslides = specs_.nrSlides();
    for ( int idx=0; idx<nrslides; idx++ )
    {
	const BufferString slidetitle = slidestbl_->text( RowCol(idx,0) );
	specs_.setSlideTitle( idx, slidetitle.buf() );
    }

    BufferString script;
    specs_.getPythonScript( script );
    FilePath scriptfp( PresentationSpec::getPyDir() );
    BufferString fnm( "python-pptx-" );
    fnm.add( Time::getDateTimeString(datefmt) );
    scriptfp.add( fnm ); scriptfp.setExtension( "py" );
    od_ostream strm( scriptfp.fullPath() );
    strm << script.buf() << od_endl;

    BufferString cmd( "python ", scriptfp.fullPath() );
    if ( !OS::ExecCommand(cmd.buf(),OS::Wait4Finish) )
    {
	uiMSG().error( tr("Could not execute\n: "), cmd.buf(),
	    tr("\nPlease check if Python is correctly installed.") );
	return;
    }

    if ( !File::exists(outputfnm) )
    {
	uiMSG().error( tr("Output file does not exist. An error might have "
	    "occured while running the python script.") );
	return;
    }

    const bool res = uiMSG().askGoOn(
	tr("Successfully created presentation. Open now?") );
    if ( !res ) return;

    uiDesktopServices::openUrl( outputfnm.buf() );
}
