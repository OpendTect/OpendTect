/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiimagesel.h"

#include "filepath.h"
#include "imagedeftr.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uifileinput.h"
#include "uimenu.h"
#include "uimsg.h"


//uiImageSel

uiImageSel::uiImageSel( uiParent* p, bool forread, const Setup& su )
    : uiIOObjSel(p,mRWIOObjContext(ImageDef,forread),su)
{
    if ( su.seltxt_.isEmpty() )
	setLabelText( forread ? uiStrings::phrInput( uiStrings::sImage() )
			      : uiStrings::phrOutput( uiStrings::sImage() ) );

    if ( !forread )
	return;

    uiPushButton* importeditbut = nullptr;
    if ( su.withimport_ && su.withedit_ )
   {
	auto* mnu = new uiMenu;
	mnu->insertAction( new uiAction(uiStrings::sImport(),
		    mCB(this,uiImageSel,doImportCB),"import") );
	mnu->insertAction( new uiAction(uiStrings::sEdit(),
		    mCB(this,uiImageSel,doEditCB),"edit") );
	auto* but =
		new uiPushButtonWithMenu( this, tr("Import/Edit"), false, mnu );
	importeditbut = but;
    }
    else if ( su.withimport_ )
    {
	importeditbut = new uiPushButton( this, uiStrings::sImport(), false );
	mAttachCB( importeditbut->activated, uiImageSel::doImportCB );
    }
    else if ( su.withedit_ )
    {
	importeditbut = new uiPushButton( this, uiStrings::sEdit(), false );
	mAttachCB( importeditbut->activated, uiImageSel::doEditCB );
    }

    if ( importeditbut )
    {
	importeditbut->attach( rightOf, this->selbut_ );
	importeditbut->setMinimumWidthInChar( importeditbut->name().size()+3 );
    }
}


uiImageSel::~uiImageSel()
{
    detachAllNotifiers();
}


void uiImageSel::doImportCB( CallBacker* )
{
    uiImportImageDlg dlg( this );
    mAttachCB( dlg.importDone, uiImageSel::importDoneCB );
    dlg.go();
}


void uiImageSel::importDoneCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack(const MultiID&,dbky,cb);
    setInput( dbky );
    selectionDone.trigger();
    if ( optbox_ )
    {
	setChecked( !dbky.isUdf() );
	optionalChecked.trigger();
    }
}


void uiImageSel::doEditCB( CallBacker* )
{
    const IOObj* curioobj = ioobj( true );
    if ( !curioobj )
    {
	uiMSG().error( tr("Please select or import an image first") );
	return;
    }

    uiEditImageDlg dlg( this, *curioobj );
    mAttachCB( dlg.editDone, uiImageSel::editDoneCB );
    dlg.go();
}


void uiImageSel::editDoneCB( CallBacker* )
{
    selectionDone.trigger();
}


// uiImageSel::Setup

uiImageSel::Setup::Setup( const uiString& seltxt )
    : uiIOObjSel::Setup(seltxt)
    , withimport_(true)
    , withedit_(true)
{}


uiImageSel::Setup::~Setup()
{
}


// uiImportImageDlg

uiImportImageDlg::uiImportImageDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Import Image"),mTODOHelpKey))
    , importDone(this)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    uiFileInput::Setup su( uiFileDialog::Img ); su.defseldir( GetDataDir() );
    inputfld_ = new uiFileInput( this, tr("Input image file"), su );
    mAttachCB( inputfld_->valueChanged, uiImportImageDlg::fileSelectedCB );

    const Coord mincrd = SI().minCoord(true);
    const Coord maxcrd = SI().maxCoord(true);
    tlcrdfld_ = new uiGenInput( this, tr("NorthWest (TopLeft) coordinate"),
                                PositionInpSpec(Coord(mincrd.x_,maxcrd.y_)) );
    tlcrdfld_->setElemSzPol( uiObject::MedVar );
    tlcrdfld_->attach( alignedBelow, inputfld_ );

    brcrdfld_ = new uiGenInput( this, tr("SouthEast (BottomRight) coordinate"),
                                PositionInpSpec(Coord(maxcrd.x_,mincrd.y_)) );
    brcrdfld_->setElemSzPol( uiObject::MedVar );
    brcrdfld_->attach( alignedBelow, tlcrdfld_ );

    outputfld_ = new uiImageSel( this, false, uiImageSel::Setup() );
    outputfld_->attach( alignedBelow, brcrdfld_ );

    mAttachCB( postFinalize(), uiImportImageDlg::finalizeCB );
}


uiImportImageDlg::~uiImportImageDlg()
{
    detachAllNotifiers();
}


void uiImportImageDlg::finalizeCB( CallBacker* )
{
    const int nrdec = SI().nrXYDecimals();
    tlcrdfld_->setNrDecimals( nrdec, 0 );
    tlcrdfld_->setNrDecimals( nrdec, 1 );
    brcrdfld_->setNrDecimals( nrdec, 0 );
    brcrdfld_->setNrDecimals( nrdec, 1 );
}


bool uiImportImageDlg::acceptOK( CallBacker* )
{
    const StringView fnm = inputfld_->fileName();
    if ( fnm.isEmpty() )
    {
	uiMSG().error( tr("Please select an image file") );
	return false;
    }

    if ( !File::exists(fnm.buf()) )
    {
	uiMSG().error( tr("Selected image file does not exist") );
	return false;
    }

    outputfld_->reset();
    const IOObj* ioobj = outputfld_->ioobj();
    if ( !ioobj )
	return false;

    PtrMan<Translator> translator = ioobj->createTranslator();
    mDynamicCastGet(ODImageDefTranslator*,imgtr,translator.ptr())
    if ( !imgtr )
	return false;

    ImageDef def;
    def.setBaseDir( IOM().rootDir().fullPath() ).setFileName( fnm );
    def.tlcoord_.coord() = tlcrdfld_->getCoord();
    def.brcoord_.coord() = brcrdfld_->getCoord();
    if ( !imgtr->write(def,*ioobj) )
	return false;

    ioobj->updateCreationPars();
    IOM().commitChanges( *ioobj );

    const MultiID dbky = ioobj->key();;
    importDone.trigger( dbky );

    uiString msg = tr("Image successfully imported."
		      "\nDo you want to import more Images?");
    return !uiMSG().askGoOn(msg, uiStrings::sYes(), tr("No, close window"));
}


void uiImportImageDlg::fileSelectedCB( CallBacker* )
{
    const FilePath fnmfp( inputfld_->fileName() );
    outputfld_->setInputText( fnmfp.baseName() );
}


// uiImageCoordGrp

uiImageCoordGrp::uiImageCoordGrp( uiParent* p )
    : uiGroup(p,"Image Coordinate Group")
{
    tlcrdfld_ = new uiGenInput( this, tr("NorthWest (TopLeft) coordinate"),
			     PositionInpSpec(Coord()) );
    tlcrdfld_->setElemSzPol( uiObject::MedVar );

    brcrdfld_ = new uiGenInput( this, tr("SouthEast (BottomRight) coordinate"),
			     PositionInpSpec(Coord()) );
    brcrdfld_->setElemSzPol( uiObject::MedVar );
    brcrdfld_->attach( alignedBelow, tlcrdfld_ );

    setHAlignObj( tlcrdfld_->attachObj() );
}


uiImageCoordGrp::~uiImageCoordGrp()
{}


void uiImageCoordGrp::fillCoords( const IOObj& ioobj )
{
    ImageDef def;
    def.setBaseDir( IOM().rootDir().fullPath() );
    ODImageDefTranslator::readDef( def, ioobj );

    tlcrdfld_->setValue( def.tlcoord_.coord() );
    brcrdfld_->setValue( def.brcoord_.coord() );

    const int nrdec = SI().nrXYDecimals();
    tlcrdfld_->setNrDecimals( nrdec, 0 );
    tlcrdfld_->setNrDecimals( nrdec, 1 );
    brcrdfld_->setNrDecimals( nrdec, 0 );
    brcrdfld_->setNrDecimals( nrdec, 1 );
}


bool uiImageCoordGrp::saveCoords( const IOObj& ioobj )
{
    ImageDef def;
    def.setBaseDir( IOM().rootDir().fullPath() );
    ODImageDefTranslator::readDef( def, ioobj );

    def.tlcoord_.coord() = tlcrdfld_->getCoord();
    def.brcoord_.coord() = brcrdfld_->getCoord();
    if ( !ODImageDefTranslator::writeDef(def,ioobj) )
	return false;

    return true;
}


void uiImageCoordGrp::fillPar( IOPar& ) const
{}


bool uiImageCoordGrp::usePar( const IOPar& )
{
    return true;
}


// uiEditImageDlg

uiEditImageDlg::uiEditImageDlg( uiParent* p, const IOObj& ioobj )
    : uiDialog(p,Setup(tr("Edit Image"),mTODOHelpKey))
    , editDone(this)
    , ioobj_(ioobj)
{
    setOkCancelText( uiStrings::sEdit(), uiStrings::sClose() );

    imagegrp_ = new uiImageCoordGrp( this );

    mAttachCB( postFinalize(), uiEditImageDlg::finalizeCB );
}


uiEditImageDlg::~uiEditImageDlg()
{
    detachAllNotifiers();
}


void uiEditImageDlg::finalizeCB( CallBacker* )
{
    imagegrp_->fillCoords( ioobj_ );
}


bool uiEditImageDlg::acceptOK( CallBacker* )
{
    const bool res = imagegrp_->saveCoords( ioobj_ );
    if ( res )
	editDone.trigger();

    return res;
}
