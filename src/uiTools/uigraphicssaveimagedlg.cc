/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigraphicssaveimagedlg.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uigraphicsscene.h"
#include "uimain.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uipixmap.h"
#include "uispinbox.h"

#include "filepath.h"
#include "iopar.h"
#include "settings.h"


uiGraphicsSaveImageDlg::uiGraphicsSaveImageDlg( uiParent* p,
	uiGraphicsScene* scene )
    : uiSaveImageDlg(p)
    , scene_(scene)
{
    screendpi_ = sCast( float, uiMain::getMinDPI() );
    createGeomInpFlds( cliboardselfld_ );
    fileinputfld_->attach( alignedBelow, dpifld_ );

    setFldVals( nullptr );
    updateSizes();

    postFinalize().notify( mCB(this,uiGraphicsSaveImageDlg,setAspectRatio) );
    updateFilter();
    unitChg( nullptr );

    NotifyStopper ns( lockfld_->activated );
    lockfld_->setChecked( true );
    lockfld_->setSensitive( false );
}


uiGraphicsSaveImageDlg::~uiGraphicsSaveImageDlg()
{}


void uiGraphicsSaveImageDlg::getSupportedFormats( const char** imagefrmt,
						  const char** frmtdesc,
						  BufferString& filters )
{
    BufferStringSet supportedformats;
    supportedImageFormats( supportedformats );
    int idy = 0;
    while ( imagefrmt[idy] )
    {
	if ( supportedformats.isPresent( imagefrmt[idy] ) )
	{
	    if ( !filters.isEmpty() ) filters += ";;";
	    filters += frmtdesc[idy];
	}
	idy++;
    }

    uiSaveImageDlg::addPrintFmtFilters( filters );
    filters_ = filters;
}


void uiGraphicsSaveImageDlg::setAspectRatio( CallBacker* )
{
    aspectratio_ = float( scene_->width()/scene_->height() );
}


bool uiGraphicsSaveImageDlg::acceptOK( CallBacker* )
{
    if ( cliboardselfld_->isChecked() )
    {
	scene_->copyToClipBoard();
	return true;
    }

    if ( !filenameOK() )
	return false;

    MouseCursorChanger mcc( MouseCursor::Wait );

    const char* fnm = fileinputfld_->fileName();
    const int pixw = sCast(int,sizepix_.width());
    const int pixh = sCast(int,sizepix_.height());
    const int dpi = dpifld_->box()->getIntValue();
    const BufferString ext( getExtension() );
    if ( ext == "pdf" )
	scene_->saveAsPDF( fnm, pixw, pixh, dpi );
    else
	scene_->saveAsImage( fnm, pixw, pixh, dpi );

    if ( saveButtonChecked() )
	writeToSettings();

    return true;
}


void uiGraphicsSaveImageDlg::writeToSettings()
{
    IOPar iopar;
    fillPar( iopar, true );
    settings_.mergeComp( iopar, "2D" );
    if ( !settings_.write() )
	uiMSG().error( uiStrings::sCantWriteSettings() );
}


void uiGraphicsSaveImageDlg::setFldVals( CallBacker* )
{
    if ( useparsfld_->getBoolValue() )
    {
	PtrMan<IOPar> ctiopar;
	getSettingsPar( ctiopar, BufferString("2D") );
	if ( ctiopar.ptr() )
	{
	    if ( !usePar(*ctiopar) )
		useparsfld_->setValue( false );
	}
	aspectratio_ = widthfld_->box()->getFValue() /
					heightfld_->box()->getFValue();
    }
    else
    {
	aspectratio_ = float( scene_->width()/scene_->height() );
	dpifld_->box()->setValue( screendpi_ );
	setSizeInPix( int(scene_->width()), int(scene_->height()) );
    }
}
