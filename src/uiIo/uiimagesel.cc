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
#include "uimsg.h"


uiImageSel::uiImageSel( uiParent* p, bool forread, const Setup& su )
    : uiIOObjSel(p,mRWIOObjContext(ImageDef,forread),su)
{
    if ( su.seltxt_.isEmpty() )
	setLabelText( forread ? uiStrings::phrInput( uiStrings::sImage() )
			      : uiStrings::phrOutput( uiStrings::sImage() ) );

    if ( forread )
    {
	auto* but = new uiPushButton( this, uiStrings::sImport(), false );
	but->attach( rightOf, this->selbut_ );
	mAttachCB( but->activated, uiImageSel::importCB );
    }
}


uiImageSel::~uiImageSel()
{
    detachAllNotifiers();
}


void uiImageSel::importCB( CallBacker* )
{
    uiImportImageDlg dlg( this );
    if ( !dlg.go() )
	return;

    setInput( dlg.getKey() );
    selectionDone.trigger();
}



// uiImportImageDlg

uiImportImageDlg::uiImportImageDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Import Image"),mNoDlgTitle,mTODOHelpKey))
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    uiFileInput::Setup su( uiFileDialog::Img ); su.defseldir( GetDataDir() );
    inputfld_ = new uiFileInput( this, tr("Input image file"), su );
    mAttachCB( inputfld_->valueChanged, uiImportImageDlg::fileSelectedCB );

    const Coord mincrd = SI().minCoord(true);
    const Coord maxcrd = SI().maxCoord(true);
    tlcrdfld_ = new uiGenInput( this, tr("NorthWest (TopLeft) coordinate"),
			     PositionInpSpec(Coord(mincrd.x,maxcrd.y)) );
    tlcrdfld_->setElemSzPol( uiObject::MedVar );
    tlcrdfld_->attach( alignedBelow, inputfld_ );

    brcrdfld_ = new uiGenInput( this, tr("SouthEast (BottomRight) coordinate"),
			     PositionInpSpec(Coord(maxcrd.x,mincrd.y)) );
    brcrdfld_->setElemSzPol( uiObject::MedVar );
    brcrdfld_->attach( alignedBelow, tlcrdfld_ );

    outputfld_ = new uiImageSel( this, false, uiIOObjSel::Setup() );
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

    const IOObj* ioobj = outputfld_->ioobj();
    if ( !ioobj )
	return false;

    PtrMan<Translator> translator = ioobj->createTranslator();
    mDynamicCastGet(ODImageDefTranslator*,imgtr,translator.ptr())
    if ( !imgtr )
	return false;

    ImageDef def;
    def.filename_ = fnm;
    def.tlcoord_.coord() = tlcrdfld_->getCoord();
    def.brcoord_.coord() = brcrdfld_->getCoord();
    if ( !imgtr->write(def,*ioobj) )
	return false;

    ioobj->updateCreationPars();
    IOM().commitChanges( *ioobj );

    uiString msg = tr("Image successfully imported."
		      "\nDo you want to import more Images?");
    return !uiMSG().askGoOn(msg, uiStrings::sYes(), tr("No, close window"));
}


void uiImportImageDlg::fileSelectedCB( CallBacker* )
{
    const FilePath fnmfp( inputfld_->fileName() );
    outputfld_->setInputText( fnmfp.baseName() );
}


MultiID uiImportImageDlg::getKey() const
{
    return outputfld_->key();
}
