/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          October 2002
 RCS:           $Id: uiprintscenedlg.cc,v 1.38 2008-04-10 04:24:13 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiprintscenedlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "mousecursor.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiobj.h"
#include "uisoviewer.h"
#include "uispinbox.h"

#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "ptrman.h"
#include "settings.h"
#include "visscene.h"

#include <Inventor/actions/SoToVRML2Action.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/VRMLnodes/SoVRMLGroup.h>
#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/SoOutput.h>

static const char* sKeySnapshot = "snapshots";

BufferString uiPrintSceneDlg::dirname_ = "";

static const char* imageformats[] =
{ "jpg", "png", "bmp", "eps", "xpm", "xbm", 0 };

static const char* filters[] =
{
    "JPEG (*.jpg *.jpeg)",
    "PNG (*.png)",
    "Bitmap (*.bmp)",
    "EPS (*.ps *.eps)",
    "XPM (*.xpm)",
    "XBM (*.xbm)",
    0
};


static const StepInterval<float> sSizeRange(0.5,999,0.1);
static StepInterval<float> sPixelSizeRange(1,9999,1);
static const int sDefdpi = 300;


static void sPixels2Inch( const SbVec2f& from, SbVec2f& to, float dpi )
{ to = from / dpi; }

static void sInch2Pixels( const SbVec2f& from, SbVec2f& to, float dpi )
{ to = from * dpi; }

static void sCm2Inch( const SbVec2f& from, SbVec2f& to )
{ to = from / 2.54; }

static void sInch2Cm( const SbVec2f& from, SbVec2f& to )
{ to = from * 2.54; }


#define mAttachToAbove( fld ) \
	if ( fldabove ) fld->attach( alignedBelow, fldabove ); \
	fldabove = fld

uiPrintSceneDlg::uiPrintSceneDlg( uiParent* p,
				  const ObjectSet<uiSoViewer>& vwrs )
    : uiDialog(p,uiDialog::Setup("Create snapshot",
				 "Enter image size and filename","50.0.1"))
    , viewers_(vwrs)
    , screendpi_(SoOffscreenRenderer::getScreenPixelsPerInch())
    , sizepix_(*new SbVec2f)
    , sizeinch_(*new SbVec2f)
    , sizecm_(*new SbVec2f)
    , heightfld_(0)
    , scenefld_(0)
    , dovrmlfld_(0)
{
    SbViewportRegion vp;
    SoOffscreenRenderer sor( vp );
    const int nrfiletypes = sor.getNumWriteFiletypes();

    bool showvrml = false;
    Settings::common().getYN( IOPar::compKey("dTect","Enable VRML"), showvrml );
    if ( nrfiletypes==0 && !showvrml )
    {
	new uiLabel( this,
	    "No output file types found.\n"
	    "Probably, 'libsimage.so' is not installed or invalid." );
	setCtrlStyle( LeaveOnly );
	return;
    }

    SbVec2s maxres = SoOffscreenRenderer::getMaximumResolution();
    sPixelSizeRange.stop = mMIN(maxres[0],maxres[1]);

    uiParent* fldabove = 0;
    if ( viewers_.size() > 1 )
    {
	BufferStringSet scenenms;
	for ( int idx=0; idx<viewers_.size(); idx++ )
	    scenenms.add( viewers_[idx]->getScene()->name() );

	scenefld_ = new uiLabeledComboBox( this, scenenms, "Make snapshot of" );
	scenefld_->box()->selectionChanged.notify( 
					mCB(this,uiPrintSceneDlg,sceneSel) );
	mAttachToAbove( scenefld_ );
    }

    if ( showvrml && nrfiletypes )
    {
	dovrmlfld_ = new uiGenInput( this, "Type of snapshot",
				     BoolInpSpec(true,"Scene","Image") );
	dovrmlfld_->valuechanged.notify( mCB(this,uiPrintSceneDlg,typeSel) );
	dovrmlfld_->setValue( false );
	mAttachToAbove( dovrmlfld_ );
    }

    if ( nrfiletypes>0 )
    {
	widthfld_ = new uiLabeledSpinBox( this, "Width", 2 );
	widthfld_->box()->valueChanged.notify(
		mCB(this,uiPrintSceneDlg,sizeChg) );
	mAttachToAbove( widthfld_ );

	heightfld_ = new uiLabeledSpinBox( this, "Height", 2 );
	heightfld_->box()->valueChanged.notify(
		mCB(this,uiPrintSceneDlg,sizeChg) );
	mAttachToAbove( heightfld_ );

	const char* units[] = { "cm", "inches", "pixels", 0 };
	unitfld_ = new uiGenInput( this, "", StringListInpSpec(units) );
	unitfld_->setElemSzPol( uiObject::Small );
	unitfld_->valuechanged.notify( mCB(this,uiPrintSceneDlg,unitChg) );
	unitfld_->attach( rightTo, widthfld_ );

	lockfld_ = new uiCheckBox( this, "Lock aspect ratio" );
	lockfld_->setChecked( true );
	lockfld_->activated.notify( mCB(this,uiPrintSceneDlg,lockChg) );
	lockfld_->attach( alignedBelow, unitfld_ );

	dpifld_ = new uiGenInput( this, "Resolution (dpi)", 
				 IntInpSpec(mNINT(screendpi_)) );
	dpifld_->setElemSzPol( uiObject::Small );
	dpifld_->valuechanging.notify( mCB(this,uiPrintSceneDlg,dpiChg) );
	mAttachToAbove( dpifld_ );

    }

    if ( dirname_.isEmpty() )
	dirname_ = FilePath(GetDataDir()).add("Misc").fullPath();
    fileinputfld_ = new uiFileInput( this, "Select filename",
				    uiFileInput::Setup()
				    .forread(false)
				    .defseldir(dirname_)
	   			    .allowallextensions(false) );
    fileinputfld_->valuechanged.notify( mCB(this,uiPrintSceneDlg,fileSel) );
    mAttachToAbove( fileinputfld_ );

    const char* settingsdir = GetSettingsDir();
    FilePath fp;
    fp.setPath( settingsdir );
    fp.setFileName( "snapshots" );
    IOPar par;
    par.read( fp.fullPath(FilePath::Local), sKey::Pars );

    updateFilter();
    sceneSel(0);
    usePar( par );

    IOM().afterSurveyChange.notify( mCB(this,uiPrintSceneDlg,surveyChanged) );
}


uiPrintSceneDlg::~uiPrintSceneDlg()
{
    delete &sizepix_, &sizeinch_, &sizecm_;
}


void uiPrintSceneDlg::surveyChanged( CallBacker* )
{
    dirname_ = "";
}


void uiPrintSceneDlg::updateFilter()
{
    BufferString filter;
    const bool vrml = dovrmlfld_ && dovrmlfld_->getBoolValue();
    if ( dovrmlfld_ && dovrmlfld_->getBoolValue() )
	filter = "VRML (*.wrl)";
    else
    {
	SbViewportRegion vp;
	SoOffscreenRenderer sor( vp );
	int idx = 0;
	while ( imageformats[idx] )
	{
	    if ( sor.isWriteSupported(imageformats[idx]) )
	    {
		if ( !filter.isEmpty() ) filter += ";;";
		filter += filters[idx];
	    }
	    idx++;
	}
    }

    fileinputfld_->setFilter( filter );
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
    const uiSoViewer* vwr = viewers_[vwridx];
    const SbVec2s& winsz = vwr->getViewportSizePixels();
    aspectratio_ = (float)winsz[0] / winsz[1];
    sizepix_.setValue( winsz[0], winsz[1] );
    sPixels2Inch( sizepix_, sizeinch_, dpifld_->getfValue() );
    sInch2Cm( sizeinch_, sizecm_ );
    unitChg(0);
}


void uiPrintSceneDlg::sizeChg( CallBacker* cb )
{
    mDynamicCastGet(uiSpinBox*,box,cb);
    if ( !box ) return;

    if ( lockfld_->isChecked() )
    {
	if ( box == widthfld_->box() )
	    heightfld_->box()->setValue(
		    widthfld_->box()->getFValue()/aspectratio_ );
	else
	    widthfld_->box()->setValue(
		    heightfld_->box()->getFValue()*aspectratio_ );
    }

    updateSizes();
}


void uiPrintSceneDlg::unitChg( CallBacker* )
{
    SbVec2f size;
    int nrdec = 2;
    StepInterval<float> range = sSizeRange;

    const int sel = unitfld_->getIntValue();
    if ( !sel )
	size = sizecm_;
    else if ( sel == 1 )
	size = sizeinch_;
    else if ( sel == 2 )
    {
	size = sizepix_;
	nrdec = 0;
	range = sPixelSizeRange;
    }

    widthfld_->box()->setNrDecimals( nrdec );
    widthfld_->box()->setInterval( range );
    widthfld_->box()->setValue( size[0] );

    heightfld_->box()->setNrDecimals( nrdec );
    heightfld_->box()->setInterval( range );
    heightfld_->box()->setValue( size[1] );
}


void uiPrintSceneDlg::lockChg( CallBacker* )
{
    if ( !lockfld_->isChecked() ) return;

    const float width = widthfld_->box()->getFValue();
    heightfld_->box()->setValue( width / aspectratio_ );
    updateSizes();
}


void uiPrintSceneDlg::dpiChg( CallBacker* )
{
    updateSizes();
}


void uiPrintSceneDlg::updateSizes()
{
    const float width = widthfld_->box()->getFValue();
    const float height = heightfld_->box()->getFValue();
    const int sel = unitfld_->getIntValue();
    if ( !sel )
    {
	sizecm_.setValue( width, height );
	sCm2Inch( sizecm_, sizeinch_ );
	sInch2Pixels( sizeinch_, sizepix_, dpifld_->getfValue() );
    }
    else if ( sel == 1 )
    {
	sizeinch_.setValue( width, height );
	sInch2Cm( sizeinch_, sizecm_ );
	sInch2Pixels( sizeinch_, sizepix_, dpifld_->getfValue() );
    }
    else
    {
	sizepix_.setValue( width, height );
	sPixels2Inch( sizepix_, sizeinch_, dpifld_->getfValue() );
	sInch2Cm( sizeinch_, sizecm_ );
    }
}


void uiPrintSceneDlg::fileSel( CallBacker* )
{
    BufferString filename = fileinputfld_->fileName();
    addFileExtension( filename );
    fileinputfld_->setFileName( filename );
}


void uiPrintSceneDlg::addFileExtension( BufferString& filename )
{
    FilePath fp( filename.buf() );
    fp.setExtension( getExtension() );
    filename = fp.fullPath();
}


bool uiPrintSceneDlg::filenameOK() const
{
    BufferString filename = fileinputfld_->fileName();
    if ( filename.isEmpty() )
    {
	uiMSG().error( "Please select filename" );
	return false;
    }

    if ( File_exists(filename) )
    {
	BufferString msg = "The file "; msg += filename; 
	if ( !File_isWritable(filename) )
	{
	    msg += " is not writable";
	    uiMSG().error(msg);
	    return false;
	}
	
	msg += " exists. Overwrite?";
	if ( !uiMSG().askGoOn(msg,true) )
	    return false;
    }

    return true;
}


bool uiPrintSceneDlg::acceptOK( CallBacker* )
{
    if ( !filenameOK() ) return false;

    const int vwridx = scenefld_ ? scenefld_->box()->currentItem() : 0;
    const uiSoViewer* vwr = viewers_[vwridx];
    FilePath filepath( fileinputfld_->fileName() );
    dirname_ = filepath.pathOnly();

    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    if ( dovrmlfld_ && dovrmlfld_->getBoolValue() )
    {
	if ( !uiMSG().askGoOn("The VRML output in in pre apha testing "
		    	      "status,\nis not officially supported and is \n"
			      "known to be very unstable.\n\n"
			      "Do you want to continue?") )
	{
	    return false;
	}

	SoToVRML2Action tovrml2;
	SoNode* root = vwr->getSceneGraph();
	root->ref();
	tovrml2.apply( root );
	SoVRMLGroup* newroot = tovrml2.getVRML2SceneGraph();
	newroot->ref();
	root->unref();

	SoOutput out;
	if ( !out.openFile( filepath.fullPath().buf() ) )
	{
	    BufferString msg =  "Cannot open file ";
	    msg += filepath.fullPath();
	    msg += ".";
	    
	    uiMSG().error( msg );
	    return false;
	}

	out.setHeaderString("#VRML V2.0 utf8");
	SoWriteAction wra(&out);
	wra.apply(newroot);
        out.closeFile();

	newroot->unref();
	return true;
    }


    if ( !widthfld_ ) return true;

    SbViewportRegion viewport;
    viewport.setWindowSize( mNINT(sizepix_[0]), mNINT(sizepix_[1]) );
    viewport.setPixelsPerInch( dpifld_->getfValue() );

    if ( uiMSG().askGoOn(" Do you want to save the settings " ) )
	write2settings();
	
    PtrMan<SoOffscreenRenderer> sor = new SoOffscreenRenderer(viewport);

#define col2f(rgb) float(col.rgb())/255
    const Color col = vwr->getBackgroundColor();
    sor->setBackgroundColor( SbColor(col2f(r),col2f(g),col2f(b)) );

    SoNode* scenegraph = vwr->getSceneGraph();
    if ( !sor->render(scenegraph) )
    {
	uiMSG().error( "Cannot render scene" );
	return false;
    }

    const char* extension = getExtension();
    if ( !sor->writeToFile(filepath.fullPath().buf(),extension) )
    {
	uiMSG().error( "Couldn't write to specified file" );
	return false;
    }

    return true;
}


void uiPrintSceneDlg::write2settings()
{
    const char* settdir = GetSettingsDir();
    FilePath fp;
    fp.setPath( settdir );
    fp.setFileName( "snapshots" );
    IOPar iopar;
    iopar.read( fp.fullPath(FilePath::Local), sKey::Pars );
    fillPar( iopar );
    if ( !iopar.write( fp.fullPath(FilePath::Local), sKey::Pars ) )
	uiMSG().error( "Cannot write in to User Settings " );
}


const char* uiPrintSceneDlg::getExtension() const
{
    if ( dovrmlfld_ && dovrmlfld_->getBoolValue() )
	return "wrl";

    int ifmt = -1;
    FilePath fp( fileinputfld_->fileName() );
    const BufferString ext( fp.extension() );
    for ( int idx=0; imageformats[idx]; idx++ )
    {
	if ( ext == imageformats[idx] )
	    { ifmt = idx; break; }
    }

    if ( ifmt < 0 )
    {
	const char* selectedfilter = fileinputfld_->selectedFilter();
	for ( ; filters[ifmt]; ifmt++ )
	{
	    if ( !strcmp(selectedfilter,filters[ifmt]) )
		break;
	}
    }

    return imageformats[ifmt] ? imageformats[ifmt] : imageformats[0];
}


void uiPrintSceneDlg::fillPar( IOPar& par ) const
{
    if ( !heightfld_ ) return;
    par.set( sKeyHeight(), heightfld_->box()->getFValue() );
    par.set( sKeyWidth(), widthfld_->box()->getFValue() );
    par.set( sKeyUnit(), unitfld_->text("Unit") );
    par.set( sKeyRes(), dpifld_->getIntValue() );
}


bool uiPrintSceneDlg::usePar( const IOPar& par )
{
    if ( !heightfld_ ) return false;

    float val;
    if ( par.get(sKeyHeight(),val) ) heightfld_->box()->setValue( val );
    if ( par.get(sKeyWidth(),val) ) widthfld_->box()->setValue( val );

    int ival;
    if ( par.get(sKeyUnit(),ival) ) unitfld_->setValue( ival );
    if ( par.get(sKeyRes(),ival) ) dpifld_->setValue( ival );
    
    sizecm_.setValue( widthfld_->box()->getFValue(),
	    	      heightfld_->box()->getFValue() );
    sCm2Inch( sizecm_, sizeinch_ );
    sInch2Pixels( sizeinch_, sizepix_, dpifld_->getfValue() );

    return true;
}
