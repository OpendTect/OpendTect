/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          October 2002
 RCS:           $Id: uiprintscenedlg.cc,v 1.13 2005-01-25 14:58:52 nanne Exp $
________________________________________________________________________

-*/

#include "uiprintscenedlg.h"
#include "uifileinput.h"
#include "iopar.h"
#include "filegen.h"
#include "filepath.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "uiobj.h"
#include "ptrman.h"

#include <Inventor/SoOffscreenRenderer.h>


const char* uiPrintSceneDlg::heightstr = "Height";
const char* uiPrintSceneDlg::widthstr = "Width";
const char* uiPrintSceneDlg::unitstr = "Unit";
const char* uiPrintSceneDlg::resstr = "Resolution";

const char* uiPrintSceneDlg::imageformats[] =
{ "jpg", "png", "bmp", "eps", "xpm", "xbm", 0 };

const char* uiPrintSceneDlg::filters[] = {
    "JPEG (*.jpg *.jpeg)",
    "PNG (*.png)",
    "Bitmap (*.bmp)",
    "EPS (*.ps *.eps)",
    "XPM (*.xpm)",
    "XBM (*.xbm)",
    0 };


const StepInterval<float> sizerange(0.5,100,0.1);
StepInterval<float> pixelsizerange(1,9000,1);


uiPrintSceneDlg::uiPrintSceneDlg( uiParent* p, SoNode* scene_,
				  const SbVec2s& winsz_ )
    : uiDialog(p,uiDialog::Setup("Print Scene",
				 "Enter filename and fileformat","50.0.1"))
    , scene(scene_)
    , heightfld(0)
    , dpi(SoOffscreenRenderer::getScreenPixelsPerInch())
    , winsz(winsz_)
{
    PtrMan<SoOffscreenRenderer> sor =
			    new SoOffscreenRenderer(*(new SbViewportRegion));
    if ( !sor->getNumWriteFiletypes() )
    {
	new uiLabel( this,
	    "No output file types found.\n"
	    "Probably, there is no valid 'libsimage.so' installed." );
	return;
    }

    SbVec2s maxres = SoOffscreenRenderer::getMaximumResolution();
    pixelsizerange.stop = mMIN(maxres[0],maxres[1]);

    uiLabeledSpinBox* wfld = new uiLabeledSpinBox( this, "Width", 2 );
    widthfld = wfld->box();
    widthfld->valueChanged.notify( mCB(this,uiPrintSceneDlg,sizeChg) );

    uiLabeledSpinBox* hfld = new uiLabeledSpinBox( this, "Height", 2 );
    heightfld = hfld->box();
    heightfld->valueChanged.notify( mCB(this,uiPrintSceneDlg,sizeChg) );
    hfld->attach( alignedBelow, wfld );

    const char* units[] = { "cm", "inches", "pixels", 0 };
    unitfld = new uiGenInput( this, "", StringListInpSpec(units) );
    unitfld->setElemSzPol( uiObject::small );
    unitfld->valuechanged.notify( mCB(this,uiPrintSceneDlg,unitChg) );
    unitfld->attach( rightTo, wfld );

    lockfld = new uiCheckBox( this, "Lock aspect ratio" );
    lockfld->setChecked( true );
    lockfld->activated.notify( mCB(this,uiPrintSceneDlg,lockChg) );
    lockfld->attach( alignedBelow, unitfld );

//  TODO: Setting the dpi does not seem to work. Wait for bugfix
/*
    dpifld = new uiGenInput( this, "Resolution (dpi)", IntInpSpec(300) );
    dpifld->setElemSzPol( uiObject::small );
    dpifld->valuechanging.notify( mCB(this,uiPrintSceneDlg,dpiChg) );
    dpifld->attach( alignedBelow, hfld );
*/

    BufferString filter;
    int idx = 0;
    while ( imageformats[idx] )
    {
	if ( idx ) filter += ";;";
	if ( sor->isWriteSupported( imageformats[idx] ) )
	    filter += filters[idx];

	idx++;
    }

    fileinputfld = new uiFileInput( this, "Select filename",
				    uiFileInput::Setup().forread(false)
				    			.filter(filter) );
    BufferString dirnm = FilePath(GetDataDir()).add("Misc").fullPath();
    fileinputfld->setDefaultSelectionDir( dirnm );
    fileinputfld->attach( alignedBelow, hfld );

    init();
    unitChg(0);
}


void uiPrintSceneDlg::init()
{
    aspectratio = (float)winsz[0] / winsz[1];
    sizepix.setValue( winsz[0], winsz[1] );
    pixels2Inch( sizepix, sizeinch );
    sizecm  = sizeinch * 2.54;
}


void uiPrintSceneDlg::pixels2Inch( const SbVec2f& from, SbVec2f& to )
{
    to = from / dpi;
}


void uiPrintSceneDlg::inch2Pixels( const SbVec2f& from, SbVec2f& to )
{
    to = from * dpi;
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
    StepInterval<float> range = sizerange;

    const int sel = unitfld->getIntValue();
    if ( !sel )
	size = sizecm;
    else if ( sel == 1 )
	size = sizeinch;
    else if ( sel == 2 )
    {
	size = sizepix;
	nrdec = 0;
	range = pixelsizerange;
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
	sizeinch = sizecm / 2.54;
	inch2Pixels( sizeinch, sizepix );
    }
    else if ( sel == 1 )
    {
	sizeinch.setValue( width, height );
	sizecm = sizeinch * 2.54;
	inch2Pixels( sizeinch, sizepix );
    }
    else
    {
	sizepix.setValue( width, height );
	pixels2Inch( sizepix, sizeinch );
	sizecm = sizeinch * 2.54;
    }
}


bool uiPrintSceneDlg::filenameOK() const
{
    const char* filename = fileinputfld->fileName();
    if ( !filename || !filename[0] )
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

    SbViewportRegion viewport;
    viewport.setWindowSize( mNINT(sizepix[0]), mNINT(sizepix[1]) );
//  viewport.setPixelsPerInch( dpifld->getfValue() );

    PtrMan<SoOffscreenRenderer> sor = new SoOffscreenRenderer(viewport);
    if ( !sor->render( scene ) )
    {
	uiMSG().error( "Cannot render scene" );
	return false;
    }

    const char* selectedfilter = fileinputfld->selectedFilter();
    int idx = 0;
    while ( filters[idx] )
    {
	if ( !strcmp(selectedfilter,filters[idx]) )
	    break;
	idx++;
    }

    const char* extension = imageformats[idx] ? imageformats[idx] 
					      : imageformats[0];
    const char* filename = fileinputfld->fileName();
    if ( !sor->writeToFile(filename,extension) )
    {
	uiMSG().error( "Couldn't write to specified file" );
	return false;
    }

    return true;
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
/*
    float val;
    if ( par.get(heightstr,val) ) heightfld->setValue(val);
    if ( par.get(widthstr,val) ) widthfld->setValue(val);

    int ival;
    if ( par.get(unitstr,ival) ) unitfld->setValue(ival);
*/
    return true;
}



