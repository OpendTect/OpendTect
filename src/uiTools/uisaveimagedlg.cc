/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisaveimagedlg.cc,v 1.1 2009-02-18 06:35:29 cvssatyaki Exp $";

#include "uisaveimagedlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uispinbox.h"

#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "oddirs.h"
#include "settings.h"
#include <math.h>

static const char* sKeySnapshot = "snapshot";

BufferString uiSaveImageDlg::dirname_ = "";

static const char* imageformats[] =
{ "jpg", "png", "bmp", "eps", "xpm", "xbm", "pdf", "ps", 0 };

static const char* imageformatdescs[] =
{
    "JPEG (*.jpg *.jpeg)",
    "PNG (*.png)",
    "Bitmap (*.bmp)",
    "XPM (*.xpm)",
    "XBM (*.xbm)",
    0
};


static const StepInterval<float> sSizeRange(0.5,999,0.1);
static StepInterval<float> sPixelSizeRange(1,9999,1);

#define mAttachToAbove( fld ) \
	if ( fldabove ) fld->attach( alignedBelow, fldabove ); \
	fldabove = fld

uiSaveImageDlg::uiSaveImageDlg( uiParent* p, const uiDialog::Setup& su )
    : uiDialog(p,su)
    , sizesChanged(this)
    , heightfld_(0)
    , screendpi_(0)
    , type_(Image)
{
    uiParent* fldabove = 0;

    if ( dirname_.isEmpty() )
	dirname_ = FilePath(GetDataDir()).add("Misc").fullPath();
    fileinputfld_ = new uiFileInput( this, "Select filename",
				    uiFileInput::Setup()
				    .forread(false)
				    .defseldir(dirname_)
	   			    .allowallextensions(false) );
    fileinputfld_->valuechanged.notify( mCB(this,uiSaveImageDlg,fileSel) );

    useparsfld_ = new uiGenInput( this, "Use parameters from",
				  BoolInpSpec(true,"Settings","Screen") );
    useparsfld_->attach( alignedBelow, fileinputfld_ );
    useparsfld_->valuechanged.notify( mCB(this,uiSaveImageDlg,setFldVals) );

    setSaveButtonChecked( true );
    IOM().afterSurveyChange.notify( mCB(this,uiSaveImageDlg,surveyChanged) );
}


void uiSaveImageDlg::updateFilter()
{
    BufferString filterstr;
    getSupportedFormats( imageformats, imageformatdescs, filterstr );
    filters_ = filterstr;
    fileinputfld_->setFilter( filterstr );
}


void uiSaveImageDlg::sPixels2Inch( const Geom::Size2D<float>& from,
				   Geom::Size2D<float>& to, float dpi )
{
    to.setWidth( from.width() / dpi );
    to.setHeight( from.height() / dpi );
}


void uiSaveImageDlg::sInch2Pixels( const Geom::Size2D<float>& from,
				   Geom::Size2D<float>& to, float dpi )
{
    to.setWidth( from.width() * dpi );
    to.setHeight( from.height() * dpi );
}


void uiSaveImageDlg::sCm2Inch( const Geom::Size2D<float>& from,
			       Geom::Size2D<float>& to )
{
    to.setWidth( from.width() / 2.54 );
    to.setHeight( from.height() / 2.54 );
}


void uiSaveImageDlg::sInch2Cm( const Geom::Size2D<float>& from,
			       Geom::Size2D<float>& to )
{
    to.setWidth( from.width() * 2.54 );
    to.setHeight( from.height() * 2.54 );
}


void uiSaveImageDlg::createGeomInpFlds( uiParent* fldabove )
{
    widthfld_ = new uiLabeledSpinBox( this, "Width", 2 );
    widthfld_->box()->valueChanged.notify( mCB(this,uiSaveImageDlg,sizeChg) );
    mAttachToAbove( widthfld_ );

    heightfld_ = new uiLabeledSpinBox( this, "Height", 2 );
    heightfld_->box()->valueChanged.notify( mCB(this,uiSaveImageDlg,sizeChg) );
    mAttachToAbove( heightfld_ );

    const char* units[] = { "cm", "inches", "pixels", 0 };
    unitfld_ = new uiGenInput( this, "", StringListInpSpec(units) );
    unitfld_->setElemSzPol( uiObject::Small );
    unitfld_->valuechanged.notify( mCB(this,uiSaveImageDlg,unitChg) );
    unitfld_->attach( rightTo, widthfld_ );

    lockfld_ = new uiCheckBox( this, "Lock aspect ratio" );
    lockfld_->setChecked( true );
    lockfld_->activated.notify( mCB(this,uiSaveImageDlg,lockChg) );
    lockfld_->attach( alignedBelow, unitfld_ );

    dpifld_ = new uiGenInput( this, "Resolution (dpi)", 
			      IntInpSpec(mNINT(screendpi_)) );
    dpifld_->setElemSzPol( uiObject::Small );
    dpifld_->valuechanging.notify( mCB(this,uiSaveImageDlg,dpiChg) );
    mAttachToAbove( dpifld_ );
}


void uiSaveImageDlg::surveyChanged( CallBacker* )
{
    dirname_ = "";
}


void uiSaveImageDlg::sizeChg( CallBacker* cb )
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


void uiSaveImageDlg::unitChg( CallBacker* )
{
    Geom::Size2D<float> size;
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
    widthfld_->box()->setValue( size.width() );

    heightfld_->box()->setNrDecimals( nrdec );
    heightfld_->box()->setInterval( range );
    heightfld_->box()->setValue( size.height() );
}


void uiSaveImageDlg::lockChg( CallBacker* )
{
    if ( !lockfld_->isChecked() ) return;

    const float width = widthfld_->box()->getFValue();
    heightfld_->box()->setValue( width / aspectratio_ );
    updateSizes();
}


void uiSaveImageDlg::dpiChg( CallBacker* )
{
    updateSizes();
}


void uiSaveImageDlg::updateSizes()
{
    const float width = widthfld_->box()->getFValue();
    const float height = heightfld_->box()->getFValue();
    const int sel = unitfld_->getIntValue();
    if ( !sel )
    {
	sizecm_.setWidth( width );
	sizecm_.setHeight( height );
	sCm2Inch( sizecm_, sizeinch_ );
	sInch2Pixels( sizeinch_, sizepix_, dpifld_->getfValue() );
    }
    else if ( sel == 1 )
    {
	sizeinch_.setWidth( width );
	sizeinch_.setHeight( height );
	sInch2Cm( sizeinch_, sizecm_ );
	sInch2Pixels( sizeinch_, sizepix_, dpifld_->getfValue() );
    }
    else
    {
	sizepix_.setWidth( width );
	sizepix_.setHeight( height );
	sPixels2Inch( sizepix_, sizeinch_, dpifld_->getfValue() );
	sInch2Cm( sizeinch_, sizecm_ );
    }
    sizesChanged.trigger();
}


void uiSaveImageDlg::fileSel( CallBacker* )
{
    BufferString filename = fileinputfld_->fileName();
    addFileExtension( filename );
    fileinputfld_->setFileName( filename );
}


void uiSaveImageDlg::addFileExtension( BufferString& filename )
{
    FilePath fp( filename.buf() );
    fp.setExtension( getExtension() );
    filename = fp.fullPath();
}


bool uiSaveImageDlg::filenameOK() const
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
    }

    return true;
}


const char* uiSaveImageDlg::getExtension()
{
    int ifmt = -1;
    FilePath fp( fileinputfld_->fileName() );
    const BufferString ext( fp.extension() );
    for ( int idx=0; imageformats[idx]; idx++ )
    {
	if ( ext == imageformats[idx] )
	{
	    if ( !strcmp(ext,"pdf") )
		type_ = PDF;
	    else if ( !strcmp(ext,"ps") )
		type_ = PostScript;
	    else
		type_ = Image;
	    ifmt = idx; break;
	}
    }

    if ( ifmt < 0 )
    {
	ifmt = 0;
	const char* selectedfilter = fileinputfld_->selectedFilter();
	BufferString filter;
	for ( int idx=0; idx<filters_.size(); idx++ )
	{
	    if ( filters_[idx] == ';' )
	    {
		if ( !filter.isEmpty() )
		{
		    if ( !strcmp(selectedfilter,filter.buf()) )
		    {
			selfilter_ = filter;
			break;
		    }
		    if ( filter.size()>1 )
			ifmt ++;
		}
		filter.setEmpty();
		continue;
	    }

	    char tempstr[2];
	    tempstr[0] = filters_[idx];
	    tempstr[1] = '\0';
	    filter += tempstr;
	}
    }

    return imageformats[ifmt] ? imageformats[ifmt] : imageformats[0];
}


void uiSaveImageDlg::fillPar( IOPar& par ) 
{
    if ( !heightfld_ ) return;
    par.set( sKeyType(), "2D" );
    par.set( sKeyHeight(), heightfld_->box()->getFValue() );
    par.set( sKeyWidth(), widthfld_->box()->getFValue() );
    par.set( sKeyUnit(), unitfld_->text() );
    par.set( sKeyRes(), dpifld_->getIntValue() );
    par.setYN( sKeyLockAR(), lockfld_->isChecked() );
    par.set( sKeyFileType(), getExtension() );
}


bool uiSaveImageDlg::usePar( const IOPar& par )
{
    if ( !heightfld_ ) return false;

    BufferString res;
    if ( par.get(sKeyUnit(),res) )
	unitfld_->setText( res );
    unitChg(0);

    float val;
    if ( par.get(sKeyHeight(),val) )
	heightfld_->box()->setValue( val );
    if ( par.get(sKeyWidth(),val) )
	widthfld_->box()->setValue( val );

    bool lockar = true;
    par.getYN( sKeyLockAR(), lockar );
    lockfld_->setChecked( lockar );

    int dpi;
    if ( par.get(sKeyRes(),dpi) )
	dpifld_->setValue( dpi );

    res == "";
    par.get( sKeyFileType(), res );

    int idx = 0;
    while ( imageformats[idx] )
    {
	if ( res != imageformats[idx] )
	{
	    idx++;
	    continue;
	}

	fileinputfld_->setSelectedFilter( selfilter_ );
	break;
    }

    updateSizes();
    return true;
}

void uiSaveImageDlg::setFldVals( CallBacker* )
{
}
