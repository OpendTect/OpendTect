/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicssaveimagedlg.cc,v 1.5 2009-03-10 06:35:42 cvssatyaki Exp $";

#include "uigraphicssaveimagedlg.h"

#include "uigraphicsscene.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uibutton.h"

#include "iopar.h"
#include "filepath.h"
#include "settings.h"

static const char* sKeySnapshot = "snapshot";

uiGraphicsSaveImageDlg::uiGraphicsSaveImageDlg( uiParent* p,
	uiGraphicsScene* scene )
    : uiSaveImageDlg(p)
    , scene_(scene)
{
    screendpi_ = scene->getDPI();
    createGeomInpFlds( useparsfld_ );
    fileinputfld_->attach( alignedBelow, dpifld_ );

    PtrMan<IOPar> ctiopar;
    getSettingsPar( ctiopar, BufferString("2D") );
    if ( ctiopar )
	usePar( *ctiopar );

    finaliseDone.notify( mCB(this,uiGraphicsSaveImageDlg,setAspectRatio) );
    updateFilter();
}


void uiGraphicsSaveImageDlg::getSupportedFormats( const char** imagefrmt,
						  const char** frmtdesc,
						  BufferString& filters )
{
    BufferStringSet supportedformats = scene_->supportedImageFormat();
    int idy = 0;
    while ( imagefrmt[idy] )
    {
	const int idx = supportedformats.indexOf( imagefrmt[idy] );
	if ( idx>=0 )
	{
	    if ( !filters.isEmpty() ) filters += ";;";
	    filters += frmtdesc[idy];
	}
	idy++;
    }

    filters += ";;PDF (*.pdf);;Postscript (*.ps)"; 
    filters_ = filters;
}


const char* uiGraphicsSaveImageDlg::getExtension()
{
    FilePath fp( fileinputfld_->fileName() );
    const BufferString ext( fp.extension() );
    if ( ext == "pdf" || !strncmp(fileinputfld_->selectedFilter(),"PDF",3) )
	return "pdf";

    return uiSaveImageDlg::getExtension();
}


void uiGraphicsSaveImageDlg::setAspectRatio( CallBacker* )
{
    aspectratio_ = (float) scene_->width() / scene_->height();
    initaspectratio_ = aspectratio_;
}


bool uiGraphicsSaveImageDlg::acceptOK( CallBacker* )
{
    if ( !filenameOK() ) return false;
    BufferString ext( getExtension() );
    if ( ext == "pdf" ) 
	scene_->saveAsPDF( fileinputfld_->fileName(), dpifld_->getIntValue() );
    else if ( ext == "ps" || ext == "eps" )
	scene_->saveAsPS( fileinputfld_->fileName(), dpifld_->getIntValue() );
    else
	scene_->saveAsImage( fileinputfld_->fileName(), (int)sizepix_.width(),
	       		     (int)sizepix_.height(), dpifld_->getIntValue() );

    if ( saveButtonChecked() )
	writeToSettings();
    return true;
}


void uiGraphicsSaveImageDlg::writeToSettings()
{
    PtrMan<IOPar> ctiopar;
    getSettingsPar( ctiopar, BufferString("2D") );
    if ( ctiopar.ptr() )
	fillPar( *ctiopar, true );
    settings_.mergeComp( *ctiopar, getStringFromInt(0) );
    if ( !settings_.write() )
	uiMSG().error( "Cannot write settings" );
}


void uiGraphicsSaveImageDlg::setFldVals( CallBacker* cb )
{
    if ( useparsfld_->getBoolValue() )
    {
	lockfld_->setChecked( false );
	lockfld_->setSensitive( true );
	PtrMan<IOPar> ctiopar;
	getSettingsPar( ctiopar, BufferString("2D") );
	if ( ctiopar.ptr() )
	    usePar( *ctiopar );
	aspectratio_ = (float) widthfld_->box()->getFValue() /
	    		       heightfld_->box()->getFValue();
    }
    else
    {
	lockfld_->setChecked( true );
	lockfld_->setSensitive( false );
	aspectratio_ = initaspectratio_;
	setSizeInPix( (int)scene_->width(), (int)scene_->height() );
    }
}
