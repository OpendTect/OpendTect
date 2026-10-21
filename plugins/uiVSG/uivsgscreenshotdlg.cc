/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgscreenshotdlg.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uimain.h"
#include "uimsg.h"
#include "uispinbox.h"

#include "iopar.h"
#include "ptrman.h"
#include "settings.h"

static bool prevbuttonstate = true;

// Only raster formats can be saved here (via QImage::save) - no PDF.
static const char* sPdfFrmt() { return "pdf"; }


uiVSGScreenshotDlg::uiVSGScreenshotDlg( uiParent* p, int curwidth,
					 int curheight )
    : uiSaveImageDlg(p,false,true)
    , curwidth_(curwidth)
    , curheight_(curheight)
{
    screendpi_ = mCast(float,uiMain::getMinDPI());
    aspectratio_ = curheight>0 ? mCast(float,curwidth)/curheight : 1.f;

    createGeomInpFlds( nullptr );
    fileinputfld_->setDefaultExtension( "png" );
    updateFilter();

    setFldVals( nullptr );
    setSaveButtonChecked( prevbuttonstate );
}


int uiVSGScreenshotDlg::width() const
{
    return mNINT32( sizepix_.width() );
}


int uiVSGScreenshotDlg::height() const
{
    return mNINT32( sizepix_.height() );
}


BufferString uiVSGScreenshotDlg::filename() const
{
    return fileinputfld_->fileName();
}


float uiVSGScreenshotDlg::dpi() const
{
    return dpifld_->box()->getFValue();
}


void uiVSGScreenshotDlg::getSupportedFormats( const char** imgfrmt,
					      const char** frmtdesc,
					      BufferString& filters )
{
    for ( int idx=0; imgfrmt[idx]; idx++ )
    {
	if ( BufferString(imgfrmt[idx]) == sPdfFrmt() )
	    continue;

	if ( !filters.isEmpty() )
	    filters += ";;";

	filters += frmtdesc[idx];
    }
}


void uiVSGScreenshotDlg::setFldVals( CallBacker* )
{
    if ( useparsfld_ && useparsfld_->getBoolValue() )
    {
	// "Settings" - reuse whatever was last saved for this dialog.
	lockfld_->setChecked( false );
	lockfld_->setSensitive( true );
	PtrMan<IOPar> ctiopar;
	getSettingsPar( ctiopar, BufferString("VSG") );
	if ( ctiopar.ptr() && !usePar(*ctiopar) )
	    useparsfld_->setValue( false );
    }
    else
    {
	// "Screen" - the viewer's current size, locked to avoid stretch/squeeze.
	dpifld_->box()->setValue( screendpi_ );
	setSizeInPix( curwidth_, curheight_ );
	lockfld_->setChecked( true );
	lockfld_->setSensitive( false );
    }
}


bool uiVSGScreenshotDlg::acceptOK( CallBacker* )
{
    if ( width()<1 || height()<1 )
    {
	uiMSG().error( tr("Please enter a valid width and height") );
	return false;
    }
    if ( !filenameOK() )
	return false;

    prevbuttonstate = saveButtonChecked();
    if ( prevbuttonstate )
	writeToSettings();

    return true;
}


void uiVSGScreenshotDlg::writeToSettings()
{
    IOPar iopar;
    fillPar( iopar, false );
    settings_.mergeComp( iopar, "VSG" );
    settings_.write();
}
