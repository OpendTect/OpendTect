/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicssaveimagedlg.cc,v 1.4 2009-02-20 09:21:39 cvssatyaki Exp $";

#include "uigraphicssaveimagedlg.h"

#include "uigraphicsscene.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uispinbox.h"

#include "iopar.h"
#include "filepath.h"
#include "settings.h"

static const char* sKeySnapshot = "snapshot";

uiGraphicsSaveImageDlg::uiGraphicsSaveImageDlg( uiParent* p,
	uiGraphicsScene* scene )
    : uiSaveImageDlg(p,uiDialog::Setup("Save Image As",
		"Enter image size and filename","50.0.1"))
    , scene_(scene)
{
    screendpi_ = scene->getDPI();
    createGeomInpFlds( 0 );
    fileinputfld_->attach( alignedBelow, dpifld_ );

    Settings& setts( Settings::fetch(sKeySnapshot) );
    PtrMan<IOPar> ctiopar = setts.subselect( 1 );
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
    BufferString ext( getExtension() );
    if ( ext == "pdf" ) 
	scene_->saveAsPDF( fileinputfld_->fileName(), dpifld_->getIntValue() );
    else if ( ext == "ps" || ext == "eps" )
	scene_->saveAsPS( fileinputfld_->fileName(), dpifld_->getIntValue() );
    else
	scene_->saveAsImage( fileinputfld_->fileName(), (int)sizepix_.width(),
	       		     (int)sizepix_.height(), dpifld_->getIntValue() );

    write2Dsettings();
    return true;
}


void uiGraphicsSaveImageDlg::write2Dsettings()
{
    Settings& setts( Settings::fetch(sKeySnapshot) );
    setts.clear();
    IOPar iop;
    fillPar( iop );
    setts.mergeComp( iop, getStringFromInt(0) );
    if ( !setts.write() )
	uiMSG().error( "Cannot write settings" );
}


void uiGraphicsSaveImageDlg::setFldVals( CallBacker* cb )
{
    if ( useparsfld_->getBoolValue() )
    {
	Settings& setts( Settings::fetch(sKeySnapshot) );
	PtrMan<IOPar> ctiopar = setts.subselect( 1 );
	usePar( *ctiopar );
	aspectratio_ = (float) widthfld_->box()->getFValue() /
	    		       heightfld_->box()->getFValue();
    }
    else
    {
	aspectratio_ = initaspectratio_;
	heightfld_->box()->setValue( scene_->height() );
	widthfld_->box()->setValue( scene_->width() );
	dpifld_->setValue( (int)screendpi_ );
	sizepix_.setWidth( scene_->width() );
	sizepix_.setHeight( scene_->height() );
	sPixels2Inch( sizepix_, sizeinch_, dpifld_->getfValue() );
	sInch2Cm( sizeinch_, sizecm_ );
	unitChg( 0 );
    }
}
