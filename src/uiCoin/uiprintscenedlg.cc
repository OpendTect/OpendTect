/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          October 2002
 RCS:           $Id: uiprintscenedlg.cc,v 1.25 2006-03-30 20:49:46 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiprintscenedlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uicursor.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiobj.h"
#include "uisoviewer.h"
#include "uispinbox.h"
#include "iopar.h"
#include "filegen.h"
#include "filepath.h"
#include "ptrman.h"
#include "oddirs.h"
#include "visscene.h"

#include <Inventor/SoOffscreenRenderer.h>


const char* uiPrintSceneDlg::heightstr = "Height";
const char* uiPrintSceneDlg::widthstr = "Width";
const char* uiPrintSceneDlg::unitstr = "Unit";
const char* uiPrintSceneDlg::resstr = "Resolution";

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


uiPrintSceneDlg::uiPrintSceneDlg( uiParent* p,
				  const ObjectSet<uiSoViewer>& vwrs )
    : uiDialog(p,uiDialog::Setup("Create snapshot",
				 "Enter image size and filename","50.0.1"))
    , viewers(vwrs)
    , screendpi(SoOffscreenRenderer::getScreenPixelsPerInch())
    , heightfld(0)
    , scenefld(0)
{
    SbViewportRegion vp;
    SoOffscreenRenderer sor( vp );
    const int nrfiletypes = sor.getNumWriteFiletypes();
    if ( !nrfiletypes )
    {
	new uiLabel( this,
	    "No output file types found.\n"
	    "Probably, 'libsimage.so' is not installed or invalid." );
	setCtrlStyle( LeaveOnly );
	return;
    }

    SbVec2s maxres = SoOffscreenRenderer::getMaximumResolution();
    sPixelSizeRange.stop = mMIN(maxres[0],maxres[1]);

    if ( viewers.size() > 1 )
    {
	BufferStringSet scenenms;
	for ( int idx=0; idx<viewers.size(); idx++ )
	{
	    visBase::Scene* scene = viewers[idx]->getScene();
	    scenenms.add( scene->name() );
	}

	scenefld = new uiLabeledComboBox( this, scenenms, "Make snapshot of" );
	scenefld->box()->selectionChanged.notify( 
					mCB(this,uiPrintSceneDlg,sceneSel) );
    }

    uiLabeledSpinBox* wfld = new uiLabeledSpinBox( this, "Width", 2 );
    widthfld = wfld->box();
    widthfld->valueChanged.notify( mCB(this,uiPrintSceneDlg,sizeChg) );
    if ( scenefld ) wfld->attach( alignedBelow, scenefld );

    uiLabeledSpinBox* hfld = new uiLabeledSpinBox( this, "Height", 2 );
    heightfld = hfld->box();
    heightfld->valueChanged.notify( mCB(this,uiPrintSceneDlg,sizeChg) );
    hfld->attach( alignedBelow, wfld );

    const char* units[] = { "cm", "inches", "pixels", 0 };
    unitfld = new uiGenInput( this, "", StringListInpSpec(units) );
    unitfld->setElemSzPol( uiObject::Small );
    unitfld->valuechanged.notify( mCB(this,uiPrintSceneDlg,unitChg) );
    unitfld->attach( rightTo, wfld );

    lockfld = new uiCheckBox( this, "Lock aspect ratio" );
    lockfld->setChecked( true );
    lockfld->activated.notify( mCB(this,uiPrintSceneDlg,lockChg) );
    lockfld->attach( alignedBelow, unitfld );

    dpifld = new uiGenInput( this, "Resolution (dpi)", 
	    		     IntInpSpec(mNINT(screendpi)) );
    dpifld->setElemSzPol( uiObject::Small );
    dpifld->valuechanging.notify( mCB(this,uiPrintSceneDlg,dpiChg) );
    dpifld->attach( alignedBelow, hfld );

    BufferString filter;
    int idx = 0;
    while ( imageformats[idx] )
    {
	if ( idx ) filter += ";;";
	if ( sor.isWriteSupported(imageformats[idx]) )
	    filter += filters[idx];

	idx++;
    }

    fileinputfld = new uiFileInput( this, "Select filename",
				    uiFileInput::Setup()
				    .forread(false)
				    .filter(filter)
	   			    .allowallextensions(false) );
    BufferString dirnm = FilePath(GetDataDir()).add("Misc").fullPath();
    fileinputfld->setDefaultSelectionDir( dirnm );
    fileinputfld->attach( alignedBelow, dpifld );
    fileinputfld->setReadOnly();
    fileinputfld->valuechanged.notify( mCB(this,uiPrintSceneDlg,fileSel) );

    sceneSel(0);
}


void uiPrintSceneDlg::sceneSel( CallBacker* )
{
    const int vwridx = scenefld ? scenefld->box()->currentItem() : 0;
    const uiSoViewer* vwr = viewers[vwridx];
    const SbVec2s& winsz = vwr->getViewportSizePixels();
    aspectratio = (float)winsz[0] / winsz[1];
    sizepix.setValue( winsz[0], winsz[1] );
    sPixels2Inch( sizepix, sizeinch, dpifld->getValue() );
    sInch2Cm( sizeinch, sizecm );
    unitChg(0);
}


void uiPrintSceneDlg::sizeChg( CallBacker* cb )
{
    mDynamicCastGet(uiSpinBox*,box,cb);
    if ( !box ) return;

    if ( lockfld->isChecked() )
    {
	if ( box == widthfld )
	    heightfld->setValue( widthfld->getFValue()/aspectratio );
	else
	    widthfld->setValue( heightfld->getFValue()*aspectratio );
    }

    updateSizes();
}


void uiPrintSceneDlg::unitChg( CallBacker* )
{
    SbVec2f size;
    int nrdec = 2;
    StepInterval<float> range = sSizeRange;

    const int sel = unitfld->getIntValue();
    if ( !sel )
	size = sizecm;
    else if ( sel == 1 )
	size = sizeinch;
    else if ( sel == 2 )
    {
	size = sizepix;
	nrdec = 0;
	range = sPixelSizeRange;
    }

    widthfld->setNrDecimals( nrdec );
    widthfld->setInterval( range );
    widthfld->setValue( size[0] );

    heightfld->setNrDecimals( nrdec );
    heightfld->setInterval( range );
    heightfld->setValue( size[1] );
}


void uiPrintSceneDlg::lockChg( CallBacker* )
{
    if ( !lockfld->isChecked() ) return;

    const float width = widthfld->getFValue();
    heightfld->setValue( width / aspectratio );
    updateSizes();
}


void uiPrintSceneDlg::dpiChg( CallBacker* )
{
    updateSizes();
}


void uiPrintSceneDlg::updateSizes()
{
    const float width = widthfld->getFValue();
    const float height = heightfld->getFValue();
    const int sel = unitfld->getIntValue();
    if ( !sel )
    {
	sizecm.setValue( width, height );
	sCm2Inch( sizecm, sizeinch );
	sInch2Pixels( sizeinch, sizepix, dpifld->getValue() );
    }
    else if ( sel == 1 )
    {
	sizeinch.setValue( width, height );
	sInch2Cm( sizeinch, sizecm );
	sInch2Pixels( sizeinch, sizepix, dpifld->getValue() );
    }
    else
    {
	sizepix.setValue( width, height );
	sPixels2Inch( sizepix, sizeinch, dpifld->getValue() );
	sInch2Cm( sizeinch, sizecm );
    }
}


void uiPrintSceneDlg::fileSel( CallBacker* )
{
    BufferString filename = fileinputfld->fileName();
    addFileExtension( filename );
    fileinputfld->setFileName( filename );
}


void uiPrintSceneDlg::addFileExtension( BufferString& filename )
{
    char* ptr = strrchr( filename.buf(), '.' );
    if ( !ptr )
    { filename += "." ; filename += getExtension(); }
    else
    {
	ptr++;
	bool found = false;
	int idx=0;
	while ( imageformats[idx] )
	{
	    if ( !strcmp(imageformats[idx],ptr) )
	    {
		found = true;
		break;
	    }

	    idx++;
	}

	if ( !found )
	{
	    filename += "." ;
	    filename += getExtension();
	}
	else if ( found && strcmp(ptr,getExtension()) )
	{
	    const int len = strlen( ptr );
	    filename[ filename.size()-len ] = 0;
	    filename += getExtension();
	}
    }
}


bool uiPrintSceneDlg::filenameOK() const
{
    BufferString filename = fileinputfld->fileName();
    if ( !filename.size() )
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
    if ( !widthfld ) return true;
    if ( !filenameOK() ) return false;

    uiCursorChanger cursorchanger( uiCursor::Wait );
    SbViewportRegion viewport;
    viewport.setWindowSize( mNINT(sizepix[0]), mNINT(sizepix[1]) );
    viewport.setPixelsPerInch( dpifld->getfValue() );

    const int vwridx = scenefld ? scenefld->box()->currentItem() : 0;
    const uiSoViewer* vwr = viewers[vwridx];
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
    const char* filename = fileinputfld->fileName();
    if ( !sor->writeToFile(filename,extension) )
    {
	uiMSG().error( "Couldn't write to specified file" );
	return false;
    }

    return true;
}


const char* uiPrintSceneDlg::getExtension() const
{
    const char* selectedfilter = fileinputfld->selectedFilter();
    int idx = 0;
    while ( filters[idx] )
    {
	if ( !strcmp(selectedfilter,filters[idx]) )
	    break;
	idx++;
    }

    return imageformats[idx] ? imageformats[idx] : imageformats[0];
}


void uiPrintSceneDlg::fillPar( IOPar& par ) const
{
    if ( !heightfld ) return;
    par.set( heightstr, heightfld->getValue() );
    par.set( widthstr, widthfld->getValue() );
    par.set( unitstr, unitfld->getIntValue() );
}


bool uiPrintSceneDlg::usePar( const IOPar& par )
{
    if ( !heightfld ) return false;

    float val;
    if ( par.get(heightstr,val) ) heightfld->setValue(val);
    if ( par.get(widthstr,val) ) widthfld->setValue(val);

    int ival;
    if ( par.get(unitstr,ival) ) unitfld->setValue(ival);

    return true;
}
