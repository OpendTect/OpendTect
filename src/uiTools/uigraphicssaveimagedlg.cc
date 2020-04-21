/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2009
________________________________________________________________________

-*/

#include "uigraphicssaveimagedlg.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uigraphicsscene.h"
#include "uimain.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uipixmap.h"
#include "uispinbox.h"

#include "iopar.h"
#include "settings.h"


uiGraphicsSaveImageDlg::uiGraphicsSaveImageDlg( uiParent* p,
	uiGraphicsScene* scene )
    : uiSaveImageDlg(p)
    , scene_(scene)
{
    screendpi_ = mCast( float, uiMain::getMinDPI() );
    createGeomInpFlds( cliboardselfld_ );
    inpfilefld_->attach( alignedBelow, dpifld_ );

    setFldVals( 0 );
    updateSizes();

    postFinalise().notify( mCB(this,uiGraphicsSaveImageDlg,setAspectRatio) );
    updateFilter();
    unitChg(0);

    NotifyStopper ns( lockfld_->activated );
    lockfld_->setChecked( true );
    lockfld_->setSensitive( false );
}


void uiGraphicsSaveImageDlg::setAspectRatio( CallBacker* )
{ aspectratio_ = (float) ( scene_->maxX() / scene_->maxY() ); }


bool uiGraphicsSaveImageDlg::acceptOK()
{
    if ( cliboardselfld_->isChecked() )
    {
	scene_->copyToClipBoard();
	return true;
    }

    if ( !filenameOK() ) return false;

    const char* fnm = inpfilefld_->fileName();
    const int pixw = mCast(int,sizepix_.width());
    const int pixh = mCast(int,sizepix_.height());
    const int dpi = dpifld_->box()->getIntValue();
    BufferString ext( getExtension() );
    if ( ext == "pdf" )
	scene_->saveAsPDF( fnm, pixw, pixh, dpi );
    else if ( ext == "ps" || ext == "eps" )
	scene_->saveAsPS( fnm, pixw, pixh, dpi );
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
	uiMSG().error( uiStrings::phrCannotWriteSettings() );
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
	aspectratio_ = (float)( scene_->maxX() / scene_->maxY() );
	dpifld_->box()->setValue( screendpi_ );
	setSizeInPix( scene_->nrPixX(), scene_->nrPixY() );
    }
}
