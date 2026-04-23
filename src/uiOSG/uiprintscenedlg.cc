/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprintscenedlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiobj.h"
#include "uipixmap.h"
#include "uirgbarray.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "ui3dviewer.h"

#include "filepath.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "settings.h"
#include "visscene.h"
#include "viscamera.h"

#include <osg/Camera>
#include <osg/Depth>
#include <osg/GraphicsContext>
#include <osg/Hint>
#include <osgViewer/View>
#include <osgViewer/Viewer>

#ifndef GL_COLOR_ATTACHMENT0
# define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_MULTISAMPLE
# define GL_MULTISAMPLE       0x809D
#endif

static bool prevbuttonstate = true;

#define FOREGROUND_TRANSPARENCY 128
#define BACKGROUND_TRANSPARENCY 255
#define m32BitSizeLimit 4294967294UL  //4GB osg image size limitation
#define OFFSCREEN_MSAA_SAMPLES 4

namespace OD
{

static void applyForegroundTransparency( osg::Image& img,
					 const osg::Vec4& clearcol,
					 unsigned char foregroundalpha )
{
    if ( img.getPixelFormat()!=GL_RGBA || !img.isDataContiguous() )
	return;

    const unsigned char clearR = mNINT32(clearcol.r()*255.f);
    const unsigned char clearG = mNINT32(clearcol.g()*255.f);
    const unsigned char clearB = mNINT32(clearcol.b()*255.f);
    const int nrpx = img.s() * img.t();
    unsigned char* px = img.data();
    for ( int idx=0; idx<nrpx; idx++, px+=4 )
    {
	const bool isbg = px[0]==clearR && px[1]==clearG && px[2]==clearB;
	px[3] = isbg ? 0 : foregroundalpha;
    }
}


static osg::Image* offScreenRenderViewToImage( osgViewer::View* view,
						 const uiSize& outsz,
						 unsigned char transparency )
{
    if ( !view || outsz.width()<=0 || outsz.height()<=0 )
	return nullptr;

    osg::Camera* cam = view->getCamera();
    osg::Node* scenedata = view->getSceneData();
    if ( !cam || !scenedata )
	return nullptr;

    osg::ref_ptr<osg::GraphicsContext::Traits> traits =
				new osg::GraphicsContext::Traits;
    traits->x = 0;
    traits->y = 0;
    traits->width = mNINT32( outsz.width() );
    traits->height = mNINT32( outsz.height() );
    traits->windowDecoration = false;
    traits->doubleBuffer = false;
    traits->pbuffer = true;
    traits->sharedContext = nullptr;
    traits->sampleBuffers = 1;
    traits->samples = OFFSCREEN_MSAA_SAMPLES;

    osg::ref_ptr<osg::GraphicsContext> offgc =
	osg::GraphicsContext::createGraphicsContext( traits.get() );
    if ( !offgc )
	return nullptr;

    osg::ref_ptr<osg::Image> colorimg = new osg::Image;
    colorimg->allocateImage( traits->width, traits->height, 1,
			     GL_RGBA, GL_UNSIGNED_BYTE );

    osg::ref_ptr<osg::Camera> offcam = new osg::Camera;
    offcam->setGraphicsContext( offgc.get() );
    offcam->setReferenceFrame( cam->getReferenceFrame() );
    offcam->setViewMatrix( cam->getViewMatrix() );
    offcam->setProjectionMatrix( cam->getProjectionMatrix() );
    offcam->setViewport( new osg::Viewport(0,0,traits->width,traits->height) );
    offcam->setCullMask( cam->getCullMask() );
    offcam->setClearMask( cam->getClearMask() );
    offcam->setClearColor( cam->getClearColor() );
    offcam->setClearDepth( cam->getClearDepth() );
    offcam->setClearStencil( cam->getClearStencil() );
    offcam->setComputeNearFarMode( cam->getComputeNearFarMode() );
    offcam->setRenderOrder( osg::Camera::NESTED_RENDER );
    offcam->setRenderOrder( cam->getRenderOrder(), cam->getRenderOrderNum() );
    offcam->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    offcam->attach( osg::Camera::COLOR_BUFFER0, colorimg.get() );
    offcam->attach( osg::Camera::DEPTH_BUFFER, GL_DEPTH_COMPONENT24 );
    offcam->setDrawBuffer( GL_COLOR_ATTACHMENT0 );
    offcam->setReadBuffer( GL_COLOR_ATTACHMENT0 );
    if ( cam->getStateSet() )
	offcam->setStateSet( cam->getStateSet() );

    offcam->getOrCreateStateSet()->setAttributeAndModes( new osg::Depth,
						osg::StateAttribute::ON );
    offcam->getOrCreateStateSet()->setMode( GL_CULL_FACE,
					    osg::StateAttribute::OFF );
    offcam->getOrCreateStateSet()->setMode( GL_MULTISAMPLE,
					    osg::StateAttribute::ON );
    offcam->getOrCreateStateSet()->setMode( GL_LINE_SMOOTH,
					    osg::StateAttribute::ON );
    offcam->getOrCreateStateSet()->setMode( GL_POINT_SMOOTH,
					    osg::StateAttribute::ON );
    offcam->getOrCreateStateSet()->setAttributeAndModes(
			    new osg::Hint( GL_LINE_SMOOTH_HINT, GL_NICEST ),
			    osg::StateAttribute::ON );
    offcam->getOrCreateStateSet()->setAttributeAndModes(
			    new osg::Hint( GL_POINT_SMOOTH_HINT, GL_NICEST ),
			    osg::StateAttribute::ON );

    osgViewer::Viewer offviewer;
    offviewer.setThreadingModel( osgViewer::Viewer::SingleThreaded );
    offviewer.setLight( cam->getView()->getLight() );
    offviewer.setCamera( offcam.get() );
    offviewer.setSceneData( scenedata );
    offviewer.realize();
    offviewer.frame();
    offviewer.frame();
    offgc->releaseContext();

    osg::Image* outputimage =
	(osg::Image*) colorimg->clone( osg::CopyOp::DEEP_COPY_ALL );
    if ( outputimage )
	applyForegroundTransparency( *outputimage, cam->getClearColor(),
				     transparency );
    return outputimage;
}

} // namespace OD


ui3DViewer2Image::ui3DViewer2Image( ui3DViewer& vwr, const char* imgfnm,
				    uiSize imgsz, int dpi )
    : vwr_(vwr)
    , imgfnm_(imgfnm)
    , sizepix_(imgsz)
{
    dpi_ = dpi==-1 ? vwr_.getScenesPixelDensity() : (float)dpi;

    if ( imgsz.height()==0 || imgsz.width()==0 )
	sizepix_ = vwr_.getViewportSizePixels();
}


ui3DViewer2Image::~ui3DViewer2Image()
{}


bool ui3DViewer2Image::create()
{
    const od_int64 size = mNINT64(sizepix_.width()*sizepix_.height())*4 ;
    if( size>m32BitSizeLimit )
    {
	pErrMsg( "The output image size is too big, please reduce image size\
		  or resolution" );
	return false;
    }

    bool ret = false;

    const float scenesDPI = vwr_.getScenesPixelDensity();
    const float renderDPI = dpi_;
    const bool changedpi = scenesDPI != renderDPI;
    if ( changedpi )
	vwr_.setScenesPixelDensity( renderDPI );

    osgViewer::View* mainview =
	const_cast<osgViewer::View*>( vwr_.getOsgViewerMainView() );

    osgViewer::View* hudview =
	const_cast<osgViewer::View*>( vwr_.getOsgViewerHudView() );

    OD::WheelMode curmode = vwr_.getWheelDisplayMode();
    vwr_.setWheelDisplayMode( OD::WheelMode::Never );
    osg::ref_ptr<osg::Image> hudimage = offScreenRenderViewToImage(
					hudview, FOREGROUND_TRANSPARENCY );
    vwr_.setWheelDisplayMode( curmode );

    osg::ref_ptr<osg::Image> mainviewimage = offScreenRenderViewToImage(
	mainview,BACKGROUND_TRANSPARENCY );

    if ( changedpi )
	vwr_.setScenesPixelDensity( scenesDPI );

    const int validresult = validateImages( mainviewimage, hudimage );

    if ( validresult == InvalidImages )
	return false;
    else if ( validresult == OnlyMainViewImage )
    {
	flipImageVertical( mainviewimage );
	ret = saveImages( mainviewimage, 0 );
    }
    else
    {
	flipImageVertical( hudimage );
	flipImageVertical( mainviewimage );
	ret = saveImages( mainviewimage, hudimage );
    }

    visBase::DataObject::requestSingleRedraw();
    return ret;
}


int ui3DViewer2Image::validateImages( const osg::Image* mainimage,
				      const osg::Image* hudimage )
{
    if ( !mainimage || !hasImageValidFormat(mainimage) )
    {
	errmsg_ = tr(" Image creation is failed. ");
	return InvalidImages;
    }

    bool validsize (false);
    if ( hudimage && mainimage )
    {
	validsize = mainimage->s() == hudimage->s() &&
		    mainimage->t() == hudimage->t();
	if ( !validsize )
	{
	    pErrMsg("The size of main view image is different from hud image.");
	    return OnlyMainViewImage;
	}
    }

    if ( !hudimage || !validsize || !hasImageValidFormat( hudimage) )
    {
	errmsg_ =
	   tr("Cannot include HUD items (color bar and axis) in screen shot.");
	return OnlyMainViewImage;
    }

    return MainAndHudImages;
}


bool ui3DViewer2Image::saveImages( const osg::Image* mainimg,
				   const osg::Image* hudimg )
{
    if ( !mainimg )
	return false;

    FilePath imgfp( imgfnm_ );
    const char* fmt = imgfp.extension();

    uiRGBArray rgbhudimage( true );
    if ( hudimg && hudimg->getPixelFormat() == GL_RGBA )
    {
	rgbhudimage.setSize( hudimg->s(), hudimg->t() );
	rgbhudimage.put( hudimg->data(),false,true );
    }

    uiRGBArray rgbmainimage( mainimg->getPixelFormat()==GL_RGBA );
    rgbmainimage.setSize( mainimg->s(), mainimg->t() );

    if ( !rgbmainimage.put( mainimg->data(), false, true ) )
	return false;

    if ( rgbhudimage.bufferSize()>0 )
    {
	rgbmainimage.blendWith( rgbhudimage, true,
	    FOREGROUND_TRANSPARENCY, false, true );
    }

    rgbmainimage.save( imgfnm_.buf(), fmt );

    return true;

}


osg::Image* ui3DViewer2Image::offScreenRenderViewToImage(
			    osgViewer::View* view, unsigned char transparency )
{
    return OD::offScreenRenderViewToImage( view, sizepix_, transparency );
}


bool ui3DViewer2Image::hasImageValidFormat( const osg::Image* image )
{
    bool ret = true;

    if ( ( image->getPixelFormat() != GL_RGBA &&
	 image->getPixelFormat() !=GL_RGB ) ||
	 !image->isDataContiguous() )
    {
	pErrMsg( "Image is in the wrong format." ) ;
	ret = false;
    }

    return ret;
}


void ui3DViewer2Image::flipImageVertical(osg::Image* image)
{
    if ( !image ) return;

    if ( image->getOrigin()==osg::Image::BOTTOM_LEFT )
	image->flipVertical();
}


// uiPrintSceneDlg
uiPrintSceneDlg::uiPrintSceneDlg( uiParent* p,
				  const ObjectSet<ui3DViewer>& vwrs )
    : uiSaveImageDlg( p, false )
    , viewers_( vwrs )
{
    screendpi_ = uiMain::getMinDPI();

    uiObject* fldabove = nullptr;
    if ( viewers_.size() > 1 )
    {
	uiStringSet scenenms;
	for ( int idx=0; idx<viewers_.size(); idx++ )
	    scenenms.add( viewers_[idx]->getScene()->uiName() );

	scenefld_ = new uiLabeledComboBox( this, scenenms,
					   tr("Make snapshot of") );
	scenefld_->box()->selectionChanged.notify(
					mCB(this,uiPrintSceneDlg,sceneSel) );
	if ( scenefld_->attachObj() )
	    scenefld_->attachObj()->attach( alignedBelow, fldabove );

	fldabove = scenefld_->attachObj();
    }


    if ( scenefld_ )
	createGeomInpFlds( scenefld_->attachObj() );
    else
	createGeomInpFlds( fldabove );

    fileinputfld_->attach( alignedBelow, dpifld_ );

    sceneSel(0);
    setFldVals( 0 );

    updateFilter();
    setSaveButtonChecked( prevbuttonstate );
    unitChg( 0 );
}


uiPrintSceneDlg::~uiPrintSceneDlg()
{}


void uiPrintSceneDlg::getSupportedFormats( const char** imagefrmt,
					   const char** frmtdesc,
					   BufferString& filters )
{
    BufferStringSet supportedimageformats;
    supportedImageFormats( supportedimageformats );

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
    const od_int64 size = mNINT64(sizepix_.width()*sizepix_.height())*4 ;
    if( size>m32BitSizeLimit )
    {
	pErrMsg( "The output image size is too big, please reduce image size\
		  or resolution" );
	return false;
    }

    if ( !filenameOK() )
	return false;

    bool ret = false;
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    const int vwridx = scenefld_ ? scenefld_->box()->currentItem() : 0;
    ui3DViewer* vwr = const_cast<ui3DViewer*>(viewers_[vwridx]);

    const float scenesDPI = vwr->getScenesPixelDensity();
    const float renderDPI = dpifld_->box()->getFValue();

    const bool changedpi = scenesDPI != renderDPI;

    if ( changedpi )
	vwr->setScenesPixelDensity( renderDPI );

    osgViewer::View* mainview =
	const_cast<osgViewer::View*>( vwr->getOsgViewerMainView() );

    osgViewer::View* hudview =
	const_cast<osgViewer::View*>( vwr->getOsgViewerHudView() );

    OD::WheelMode curmode = vwr->getWheelDisplayMode();
    vwr->setWheelDisplayMode( OD::WheelMode::Never );
    osg::ref_ptr<osg::Image> hudimage = offScreenRenderViewToImage(
	hudview, FOREGROUND_TRANSPARENCY );
    vwr->setWheelDisplayMode( curmode );

    osg::ref_ptr<osg::Image> mainviewimage = offScreenRenderViewToImage(
	mainview,BACKGROUND_TRANSPARENCY );

    if ( changedpi )
	vwr->setScenesPixelDensity( scenesDPI );

    const int validresult = validateImages( mainviewimage, hudimage );

    if ( validresult == InvalidImages )
	return false;
    else if ( validresult == OnlyMainViewImage )
    {
	flipImageVertical( mainviewimage );
	ret = saveImages( mainviewimage, nullptr );
    }
    else
    {
	flipImageVertical( hudimage );
	flipImageVertical( mainviewimage );
	ret = saveImages( mainviewimage, hudimage );
    }

    prevbuttonstate = saveButtonChecked();
    if ( prevbuttonstate )
	writeToSettings();

    visBase::DataObject::requestSingleRedraw();
    return ret;
}


int uiPrintSceneDlg::validateImages(const osg::Image* mainimage,
				    const osg::Image* hudimage)
{
    if ( !mainimage || !hasImageValidFormat(mainimage) )
    {
	uiMSG().error( tr(" Image creation is failed. ") );
	return InvalidImages;
    }

    bool validsize (false);
    if ( hudimage && mainimage )
    {
	validsize = mainimage->s() == hudimage->s() &&
		    mainimage->t() == hudimage->t();
	if ( !validsize )
	{
	    pErrMsg("The size of main view image is different from hud image.");
	    return OnlyMainViewImage;
	}
    }

    if ( !hudimage || !validsize || !hasImageValidFormat( hudimage) )
    {
	uiMSG().warning(
	   tr("Cannot include HUD items (color bar and axis) in screen shot."));
	return OnlyMainViewImage;
    }

    return MainAndHudImages;
}


bool uiPrintSceneDlg::saveImages( const osg::Image* mainimg,
				  const osg::Image* hudimg )
{
    if ( !widthfld_ || !mainimg )
	return false;

    FilePath filepath( fileinputfld_->fileName() );
    setDirName( filepath.pathOnly() );

    const char* fmt = uiSaveImageDlg::getExtension();

    uiRGBArray rgbhudimage( true );
    if ( hudimg && hudimg->getPixelFormat() == GL_RGBA )
    {
	rgbhudimage.setSize( hudimg->s(), hudimg->t() );
	rgbhudimage.put( hudimg->data(),false,true );
    }

    uiRGBArray rgbmainimage( mainimg->getPixelFormat()==GL_RGBA );
    rgbmainimage.setSize( mainimg->s(), mainimg->t() );

    if ( !rgbmainimage.put( mainimg->data(), false, true ) )
	return false;

    if ( rgbhudimage.bufferSize()>0 )
    {
	rgbmainimage.blendWith( rgbhudimage, true,
	    FOREGROUND_TRANSPARENCY, false, true );
    }
    rgbmainimage.save( filepath.fullPath().buf(),fmt );

    return true;

}


osg::Image* uiPrintSceneDlg::offScreenRenderViewToImage(
			    osgViewer::View* view, unsigned char transparency )
{
    const uiSize sizepix( mNINT32(sizepix_.width()),
			  mNINT32(sizepix_.height()) );
    return OD::offScreenRenderViewToImage( view, sizepix, transparency );
}


bool uiPrintSceneDlg::hasImageValidFormat( const osg::Image* image )
{
    bool ret = true;

    if ( ( image->getPixelFormat() != GL_RGBA &&
	 image->getPixelFormat() != GL_RGB ) ||
	 !image->isDataContiguous() )
    {
	pErrMsg( "Image is in the wrong format." ) ;
	ret = false;
    }

    return ret;
}


void uiPrintSceneDlg::flipImageVertical( osg::Image* image )
{
    if ( !image )
	return;

    if ( image->getOrigin()==osg::Image::BOTTOM_LEFT )
	image->flipVertical();
}


void uiPrintSceneDlg::writeToSettings()
{
    IOPar iopar;
    fillPar( iopar, false );
    settings_.mergeComp( iopar, "3D" );
    if ( !settings_.write() )
	uiMSG().error( uiStrings::sCantWriteSettings() );
}


const char* uiPrintSceneDlg::getExtension()
{
    return uiSaveImageDlg::getExtension();
}
