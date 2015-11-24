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

#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "od_ostream.h"
#include "oscommand.h"


class uiSlideLayoutDlg : public uiDialog
{ mODTextTranslationClass(uiSlideLayoutDlg)
public:
uiSlideLayoutDlg( uiParent* p, PresentationSpec& spec )
    : uiDialog(p,Setup(tr("Settings"),mNoDlgTitle,mTODOHelpKey))
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

    uiSeparator* sep2 = new uiSeparator( this, "Sep2" );
    sep2->attach( stretchedBelow, tf );

    uiLabel* mlbl = new uiLabel( this, tr("Master/Layout Index:") );
    mlbl->attach( leftBorder );
    mlbl->attach( ensureBelow, sep2 );

    titlemasterfld_ = new uiGenInput( this, tr("Title Master"), IntInpSpec() );
    titlelayoutfld_ = new uiGenInput( this, tr("Layout"), IntInpSpec() );
    masterfld_ = new uiGenInput( this, tr("Slide Master"), IntInpSpec() );
    layoutfld_ = new uiGenInput( this, tr("Layout"), IntInpSpec() );
    titlemasterfld_->attach( alignedBelow, tf );
    titlemasterfld_->attach( ensureBelow, mlbl );
    titlelayoutfld_->attach( rightTo, titlemasterfld_ );
    masterfld_->attach( alignedBelow, titlemasterfld_ );
    layoutfld_->attach( rightTo, masterfld_ );

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

    titlemasterfld_->setValue( spec_.titlemasterindex_ );
    titlelayoutfld_->setValue( spec_.titlelayoutindex_ );
    masterfld_->setValue( layout.masterindex_ );
    layoutfld_->setValue( layout.layoutindex_ );
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

    layout.masterindex_ = masterfld_->getIntValue();
    layout.layoutindex_ = layoutfld_->getIntValue();
    spec_.titlemasterindex_ = titlemasterfld_->getIntValue();
    spec_.titlelayoutindex_ = titlelayoutfld_->getIntValue();
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

    uiGenInput*		titlemasterfld_;
    uiGenInput*		titlelayoutfld_;
    uiGenInput*		masterfld_;
    uiGenInput*		layoutfld_;
};


uiPresentationMakerDlg::uiPresentationMakerDlg( uiParent* )
    : uiDialog(0,Setup(tr("Presentation Maker"),mNoDlgTitle,mTODOHelpKey))
{
    setModal( false );
    setCtrlStyle( CloseOnly );

    titlefld_ = new uiGenInput( this, tr("Presentation Title") );
    titlefld_->setElemSzPol( uiObject::Wide );

    uiToolButton* layoutbut =
	new uiToolButton( this, "settings", tr("Slide Layout"),
			  mCB(this,uiPresentationMakerDlg,layoutCB) );
    layoutbut->attach( rightTo, titlefld_ );

    BufferString filter( "PowerPoint (*.pptx)" );
    uiFileInput::Setup fis;
    fis.forread(true).filter( filter );
    masterfld_ = new uiFileInput( this, tr("Master pptx"), fis );
    masterfld_->attach( alignedBelow, titlefld_ );

    fis.forread(false);
    outputfld_ = new uiFileInput( this, tr("Output pptx"), fis );
    outputfld_->attach( alignedBelow, masterfld_ );

    const BufferString imgpath =
		FilePath( GetDataDir() ).add( "Misc" ).fullPath();
    fis.forread(true).directories(true).filter("");
    imagestorfld_ = new uiFileInput( this, tr("Image Storage Location"), fis );
    imagestorfld_->setFileName( imgpath.buf() );
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
    slidestbl_->attach( ensureBelow, windowfld_ );

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


void uiPresentationMakerDlg::layoutCB( CallBacker* )
{
    uiSlideLayoutDlg dlg( this, specs_ );
    dlg.go();
}


static int slideidx = 1;
void uiPresentationMakerDlg::addCB( CallBacker* )
{
    FilePath imagefp( imagestorfld_->fileName() );
    imagefp.add( BufferString("image",slideidx) );
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


	uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
	const int visid = visserv->getSelObjectId();
	const int attrib = visserv->getSelAttribNr();
	uiString objnm = visserv->getObjectName( visid );
	slidename = objnm.getFullString();

	if ( attrib >= 0 )
	{
	    uiString dispnm =
		uiODAttribTreeItem::createDisplayName( visid, attrib );
	    slidename.add( " : " );
	    slidename.add( dispnm.getFullString() );
	}
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
}


void uiPresentationMakerDlg::moveDownCB( CallBacker* )
{
    const int currow = slidestbl_->currentRow();
    if ( currow > slidestbl_->nrRows() -1 ) return;
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

    const BufferString outputfnm = outputfld_->fileName();
    if ( outputfnm.isEmpty() )
    {
	uiMSG().error( tr("Please provide output file name") );
	return;
    }

    specs_.setTitle( title );
    specs_.setOutputFilename( outputfnm );

    const BufferString masterfnm = masterfld_->fileName();
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

