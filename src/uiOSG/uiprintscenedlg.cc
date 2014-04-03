/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          October 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiprintscenedlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "mousecursor.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiobj.h"
#include "ui3dviewer.h"
#include "uispinbox.h"

#include "filepath.h"
#include "keystrs.h"
#include "settings.h"
#include "visscene.h"
#include "uimain.h"
#include "visscene.h"
#include "viscamera.h"

#include "uirgbarray.h"
#include <osgGeo/TiledOffScreenRenderer>


#define mAttachToAbove( fld ) \
	if ( fldabove ) fld->attach( alignedBelow, fldabove ); \
	fldabove = fld


uiPrintSceneDlg::uiPrintSceneDlg( uiParent* p,
				  const ObjectSet<ui3DViewer>& vwrs )
    : uiSaveImageDlg(p,false)
    , viewers_(vwrs)
    , scenefld_(0)
    , dovrmlfld_(0)
    , selfld_(0)
{
    screendpi_ = uiMain::getDPI();
    const int nrfiletypes = 1; // at least one image type is supported by osg

    bool showvrml = false;
    Settings::common().getYN( IOPar::compKey("dTect","Enable VRML"), showvrml );
    if ( nrfiletypes==0 && !showvrml )
    {
	uiLabel* label mUnusedVar = new uiLabel( this,
	    "No output file types found.\n"
	    "Probably, 'libsimage.so' is not installed or invalid." );
	setCtrlStyle( RunAndClose );
	return;
    }


    uiObject* fldabove = 0;
    if ( viewers_.size() > 1 )
    {
	BufferStringSet scenenms;
	for ( int idx=0; idx<viewers_.size(); idx++ )
	    scenenms.add( viewers_[idx]->getScene()->name() );

	scenefld_ = new uiLabeledComboBox( this, scenenms, "Make snapshot of" );
	scenefld_->box()->selectionChanged.notify(
					mCB(this,uiPrintSceneDlg,sceneSel) );
	mAttachToAbove( scenefld_->attachObj() );
    }

    if ( showvrml && nrfiletypes )
    {
	dovrmlfld_ = new uiGenInput( this, "Type of snapshot",
				     BoolInpSpec(true,"Scene","Image") );
	dovrmlfld_->valuechanged.notify( mCB(this,uiPrintSceneDlg,typeSel) );
	dovrmlfld_->setValue( false );
	mAttachToAbove( dovrmlfld_->attachObj() );
    }

    if ( nrfiletypes>0 )
    {
	if ( showvrml )
	    createGeomInpFlds( dovrmlfld_->attachObj() );
	else if ( scenefld_ )
	    createGeomInpFlds( scenefld_->attachObj() );
	else
	    createGeomInpFlds( fldabove );
    }
    fileinputfld_->attach( alignedBelow, dpifld_ );

    sceneSel(0);
    PtrMan<IOPar> ctiopar;
    getSettingsPar( ctiopar, BufferString("3D") );

    if ( ctiopar.ptr() )
    {
	if ( !usePar(*ctiopar) )
	    useparsfld_->setValue( false );
    }
    else
    {
	useparsfld_->setValue( false );
	setFldVals( 0 );
    }

    updateFilter();

    if ( nrfiletypes>0 )
	unitChg( 0 );
}


void uiPrintSceneDlg::getSupportedFormats( const char** imagefrmt,
					   const char** frmtdesc,
					   BufferString& filters )
{
    BufferStringSet supportedimageformats;
    ioPixmap::supportedImageFormats( supportedimageformats );

    int idx = 0;
    while ( imagefrmt[idx] )
    {
	for ( int idxfmt = 0; idxfmt<supportedimageformats.size(); idxfmt++ )
	{
	    if ( supportedimageformats.get(idxfmt) == imagefrmt[idx] )
	    {
		if ( !filters.isEmpty() ) filters += ";;";
		filters += frmtdesc[idx];
		break;
	    }
	}
	idx++;
    }

}


void uiPrintSceneDlg::setFldVals( CallBacker* )
{
    if ( useparsfld_->getBoolValue() )
    {
	lockfld_->setChecked( false );
	lockfld_->setSensitive( true );
	PtrMan<IOPar> ctiopar;
	getSettingsPar( ctiopar, BufferString("3D") );

	if ( ctiopar.ptr() )
	{
	    if ( !usePar(*ctiopar) )
		useparsfld_->setValue( false );
	}
    }
    else
    {
	dpifld_->box()->setValue( screendpi_ );
	sceneSel( 0 );
	lockfld_->setChecked( true );
	lockfld_->setSensitive( false );
    }
}


void uiPrintSceneDlg::typeSel( CallBacker* )
{
    const bool dovrml = dovrmlfld_->getBoolValue();
    if ( heightfld_ )	heightfld_->display( !dovrml );
    if ( widthfld_ )	widthfld_->display( !dovrml );
    if ( unitfld_ )	unitfld_->display( !dovrml );
    if ( lockfld_ )	lockfld_->display( !dovrml );
    if ( dpifld_ )	dpifld_->display( !dovrml );
    updateFilter();
}


void uiPrintSceneDlg::sceneSel( CallBacker* )
{
    const int vwridx = scenefld_ ? scenefld_->box()->currentItem() : 0;
    const ui3DViewer* vwr = viewers_[vwridx];
    const Geom::Size2D<int> winsz = vwr->getViewportSizePixels();
    aspectratio_ = (float)winsz.width() / winsz.height();

    if ( useparsfld_->getBoolValue() )
	return;

    setSizeInPix( winsz.width(), winsz.height() );
}


bool uiPrintSceneDlg::acceptOK( CallBacker* )
{
    if ( !filenameOK() || !widthfld_ ) 
	return false;

    FilePath filepath( fileinputfld_->fileName() );
    setDirName( filepath.pathOnly() );

    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    const int vwridx = scenefld_ ? scenefld_->box()->currentItem() : 0;
    const ui3DViewer* vwr = viewers_[vwridx];
    osg::View* osgview =  
	vwr->getScene()->getCamera()->osgCamera()->getView();
    mDynamicCastGet( osgViewer::View*,view,osgview);

    osgGeo::TiledOffScreenRenderer offrenderer( view,
				visBase::DataObject::getCommonViewer() );

    offrenderer.setOutputSize( mNINT32(sizepix_.width()), 
	mNINT32(sizepix_.height()) );

    if ( offrenderer.createOutput() )
    {
	osg::ref_ptr<osg::Image> outputimage = (osg::Image*) 
	    offrenderer.getOutput()->clone(osg::CopyOp::DEEP_COPY_ALL);
	
	if ( !outputimage || (outputimage->getPixelFormat()   !=
	    GL_RGBA && outputimage->getPixelFormat()!=GL_RGB ) || 
	    !outputimage->isDataContiguous() )
	{
	    pErrMsg("Image is in the wrong format");
	    return false;
	}

	uiRGBArray rgbimage(outputimage->getPixelFormat()==GL_RGBA);
	rgbimage.setSize( outputimage->s(), outputimage->t() );

	if ( outputimage->getOrigin()==osg::Image::BOTTOM_LEFT )
	    outputimage->flipVertical();

	rgbimage.put( outputimage->data(), false, true );

	const char* fmt = uiSaveImageDlg::getExtension();
	return rgbimage.save( filepath.fullPath().buf(),fmt );
    }

    return false;
}


void uiPrintSceneDlg::writeToSettings()
{
    IOPar iopar;
    fillPar( iopar, false );
    settings_.mergeComp( iopar, "3D" );
    if ( !settings_.write() )
	uiMSG().error( "Cannot write settings" );
}


const char* uiPrintSceneDlg::getExtension()
{
    return uiSaveImageDlg::getExtension();
}

