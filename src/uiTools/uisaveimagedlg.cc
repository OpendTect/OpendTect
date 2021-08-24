/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2009
________________________________________________________________________

-*/

#include "uisaveimagedlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uiseparator.h"
#include "uispinbox.h"

#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "settings.h"

#include <math.h>
#include <string.h>

static const char* sKeySnapshot = "snapshot";

BufferString uiSaveImageDlg::dirname_;

static int sPDFfmtIdx = 6;
static int sPSfmtIdx = 7;
static int sEPSfmtIdx = 8;

static const char* imageformats[] =
{ "jpg", "png", "tiff", "webp", "bmp", "xpm", "pdf", "ps", "eps", 0 };

static const char* imageformatdescs[] =
{
    "JPEG (*.jpg *.jpeg)",
    "PNG (*.png)",
    "TIFF (*.tif *.tiff)",
    "WebP (*.webp)",
    "Windows Bitmap (*.bmp)",
    "XPM (*.xpm)",
    "Portable Doc Format (*.pdf)",
    "Postscript (*.ps)",
    "EPS (*.eps)",
    0
};


static const StepInterval<float> maximum_size_range(0.5,999,0.1);
static StepInterval<float> maximum_pixel_size_range(1,99999,1);
static StepInterval<int> maximum_dpi_range(1,9999,1);


uiSaveImageDlg::uiSaveImageDlg( uiParent* p, bool withclipbrd, bool withparsfld)
    : uiDialog(p,uiDialog::Setup(tr("Create snapshot"),mNoDlgTitle,
				 mODHelpKey(mPrintSceneDlgHelpID))
		 .savebutton(true))
    , sizesChanged(this)
    , heightfld_(0)
    , screendpi_(0)
    , withuseparsfld_(withparsfld)
    , useparsfld_(0)
    , aspectratio_(1.0f)
    , settings_( Settings::fetch(sKeySnapshot) )
{
    if ( withclipbrd )
    {
	cliboardselfld_ =
		new uiCheckBox( this, tr("Copy to Clipboard (Ctrl-C)") );
	cliboardselfld_->setChecked( false );
	cliboardselfld_->activated.notify(
		mCB(this,uiSaveImageDlg,copyToClipBoardClicked) );
    }

    setSaveButtonChecked( true );

    IOM().afterSurveyChange.notify( mCB(this,uiSaveImageDlg,surveyChanged) );
}


void uiSaveImageDlg::setDirName( const char* nm )
{
    dirname_ = nm;
}


void uiSaveImageDlg::copyToClipBoardClicked( CallBacker* )
{
    if ( !cliboardselfld_ ) return;

    const bool disp = !cliboardselfld_->isChecked();

    pixheightfld_->display( disp );
    pixwidthfld_->display( disp );
    heightfld_->display( disp );
    widthfld_->display( disp );
    dpifld_->display( disp );
    unitfld_->display( disp );
    lockfld_->display( disp );
    if ( useparsfld_ ) useparsfld_->display( disp );
    fileinputfld_->display( disp );
    pixlable_->display( disp );
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
    to.setWidth( from.width() / 2.54f );
    to.setHeight( from.height() / 2.54f );
}


void uiSaveImageDlg::sInch2Cm( const Geom::Size2D<float>& from,
			       Geom::Size2D<float>& to )
{
    to.setWidth( from.width() * 2.54f );
    to.setHeight( from.height() * 2.54f );
}


void uiSaveImageDlg::createGeomInpFlds( uiObject* fldabove )
{
    if ( withuseparsfld_ )
    {
	useparsfld_ = new uiGenInput( this, tr("Get size from"),
		BoolInpSpec(false,uiStrings::sSettings(),tr("Screen")) );
	useparsfld_->valuechanged.notify( mCB(this,uiSaveImageDlg,setFldVals) );
	if ( fldabove ) useparsfld_->attach( alignedBelow, fldabove );
    }

    pixwidthfld_ = new uiLabeledSpinBox( this, tr("Width"), 2 );
    pixwidthfld_->box()->setInterval( maximum_pixel_size_range );
    pixwidthfld_->box()->setNrDecimals( 0 );
    pixwidthfld_->box()->valueChanging.notify(mCB(this,uiSaveImageDlg,sizeChg));
    if ( useparsfld_ )
	pixwidthfld_->attach( alignedBelow, useparsfld_ );
    else
	pixwidthfld_->attach( alignedBelow, fldabove );

    pixheightfld_ = new uiLabeledSpinBox( this, tr("Height"), 2 );
    pixheightfld_->box()->setInterval( maximum_pixel_size_range );
    pixheightfld_->box()->setNrDecimals( 0 );
    pixheightfld_->box()->valueChanging.notify(
	    mCB(this,uiSaveImageDlg,sizeChg) );
    pixheightfld_->attach( rightTo, pixwidthfld_ );

    pixlable_ = new uiLabel( this, tr("pixels") );
    pixlable_->attach( rightTo, pixheightfld_ );

    widthfld_ = new uiLabeledSpinBox( this, tr("Width"), 2 );
    widthfld_->box()->setInterval( maximum_size_range );
    widthfld_->box()->setNrDecimals( 2 );
    widthfld_->box()->valueChanging.notify( mCB(this,uiSaveImageDlg,sizeChg) );
    widthfld_->attach( alignedBelow, pixwidthfld_ );

    heightfld_ = new uiLabeledSpinBox( this, tr("Height"), 2 );
    heightfld_->box()->setInterval( maximum_size_range );
    heightfld_->box()->setNrDecimals( 2 );
    heightfld_->box()->valueChanging.notify( mCB(this,uiSaveImageDlg,sizeChg) );
    heightfld_->attach( rightTo, widthfld_ );

    const char* units[] = { "cm", "inches", 0 };
    unitfld_ = new uiGenInput( this, uiStrings::sEmptyString(),
						    StringListInpSpec(units) );
    unitfld_->setElemSzPol( uiObject::Small );
    unitfld_->valuechanged.notify( mCB(this,uiSaveImageDlg,unitChg) );
    unitfld_->attach( rightTo, heightfld_ );

    lockfld_ = new uiCheckBox( this, tr("Lock aspect ratio") );
    lockfld_->setChecked( false );
    lockfld_->activated.notify( mCB(this,uiSaveImageDlg,lockChg) );
    lockfld_->attach( alignedBelow, unitfld_ );

    dpifld_ = new uiLabeledSpinBox( this, tr("Resolution (dpi)") );
    dpifld_->box()->setValue( (int)screendpi_ );
    dpifld_->box()->valueChanging.notify( mCB(this,uiSaveImageDlg,dpiChg) );
    dpifld_->attach( alignedBelow, widthfld_ );

    if ( dirname_.isEmpty() )
	dirname_ = GetSurveyPicturesDir();
    fileinputfld_ = new uiFileInput( this, uiStrings::phrSelect(tr("filename")),
				    uiFileInput::Setup(uiFileDialog::Gen)
				    .forread(false)
				    .defseldir(dirname_)
				    .directories(false)
				    .allowallextensions(false) );
    fileinputfld_->setDefaultExtension( "jpg" );
    fileinputfld_->valuechanged.notify( mCB(this,uiSaveImageDlg,fileSel) );
    fileinputfld_->attach( alignedBelow, dpifld_ );
}


void uiSaveImageDlg::surveyChanged( CallBacker* )
{
    dirname_ = "";
}


void uiSaveImageDlg::setNotifiers( bool enable )
{
    if ( enable )
    {
	widthfld_->box()->valueChanging.enable();
	heightfld_->box()->valueChanging.enable();
	pixwidthfld_->box()->valueChanging.enable();
	pixheightfld_->box()->valueChanging.enable();
    }
    else
    {
	widthfld_->box()->valueChanging.disable();
	heightfld_->box()->valueChanging.disable();
	pixwidthfld_->box()->valueChanging.disable();
	pixheightfld_->box()->valueChanging.disable();
    }
}


void uiSaveImageDlg::sizeChg( CallBacker* cb )
{
    mDynamicCastGet(uiSpinBox*,box,cb);
    if ( !box ) return;

    setNotifiers( false );

    bool updatepixfld = box == pixheightfld_->box() ||
			box == pixwidthfld_->box();
    if ( lockfld_->isChecked() )
    {
	if ( box == widthfld_->box() )
	    heightfld_->box()->setValue(
		    widthfld_->box()->getFValue()/aspectratio_ );
	else if ( box == heightfld_->box() )
	    widthfld_->box()->setValue(
		    heightfld_->box()->getFValue()*aspectratio_ );
	else if ( box == pixwidthfld_->box() )
	    pixheightfld_->box()->setValue(
		    pixwidthfld_->box()->getFValue()/aspectratio_ );
	else if ( box == pixheightfld_->box() )
	    pixwidthfld_->box()->setValue(
		    pixheightfld_->box()->getFValue()*aspectratio_ );
    }

    if ( updatepixfld )
    {
	sizepix_.setHeight( pixheightfld_->box()->getFValue() );
	sizepix_.setWidth( pixwidthfld_->box()->getFValue() );
	setSizeInPix( (int)sizepix_.width(), (int)sizepix_.height() );
    }
    else
    {
	fldranges_.start = heightfld_->box()->getFValue();
	fldranges_.stop = widthfld_->box()->getFValue();
	updateSizes();
    }

    setNotifiers( true );
}


void uiSaveImageDlg::unitChg( CallBacker* )
{
    setNotifiers( false );

    const int sel = unitfld_->getIntValue();
    Geom::Size2D<float> size = !sel ? sizecm_ : sizeinch_;

    widthfld_->box()->setValue( size.width() );
    fldranges_.stop = size.width();

    heightfld_->box()->setValue( size.height() );
    fldranges_.start = size.height();

    updateSizes();
    setNotifiers( true );
}


void uiSaveImageDlg::lockChg( CallBacker* )
{
    if ( !lockfld_->isChecked() ) return;

    const float width = widthfld_->box()->getFValue();
    heightfld_->box()->setValue( width / aspectratio_ );
    fldranges_.start = (float) ( width / aspectratio_ );
    updateSizes();
}


void uiSaveImageDlg::dpiChg( CallBacker* )
{
    setNotifiers( false );
    updateSizes();
    setNotifiers( true );
}


void uiSaveImageDlg::updateSizes()
{
    const float width = fldranges_.stop;
    const float height = fldranges_.start;
    if ( mIsZero(width,mDefEps) || mIsZero(height,mDefEps) )
	return;

    const int sel = unitfld_->getIntValue();
    const float dpi = dpifld_->box()->getFValue();
    if ( !sel )
    {
	sizecm_.setWidth( width );
	sizecm_.setHeight( height );
	sCm2Inch( sizecm_, sizeinch_ );
	sInch2Pixels( sizeinch_, sizepix_, dpi );
    }
    else if ( sel == 1 )
    {
	sizeinch_.setWidth( width );
	sizeinch_.setHeight( height );
	sInch2Cm( sizeinch_, sizecm_ );
	sInch2Pixels( sizeinch_, sizepix_, dpi );
    }

    pixwidthfld_->box()->setValue( sizepix_.width() );
    pixheightfld_->box()->setValue( sizepix_.height() );

    sizesChanged.trigger();
}


void uiSaveImageDlg::fileSel( CallBacker* )
{
    BufferString filename = fileinputfld_->fileName();
    if ( filename.isEmpty() ) return;

    if ( !File::isDirectory(filename) )
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
	uiMSG().error( tr("Please select filename") );
	return false;
    }

    if (File::exists(filename) && !File::isWritable(filename))
    {
	uiString msg = tr("The file %1 is not writable").arg(filename);
	    uiMSG().error(msg);
	    return false;
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
	    { ifmt = idx; break; }
    }

    if ( ifmt < 0 )
    {
	ifmt = 0;
	selfilter_ = fileinputfld_->selectedFilter();
	BufferString filter;
	for ( int idx=0; idx<filters_.size(); idx++ )
	{
	    if ( filters_[idx] == ';' )
	    {
		if ( !filter.isEmpty() )
		{
		    if ( filter == selfilter_ )
			break;
		}
		filter.setEmpty();
		continue;
	    }

	    char tempstr[2];
	    tempstr[0] = filters_[idx];
	    tempstr[1] = '\0';
	    filter += tempstr;
	}

	for ( int idx=0; imageformatdescs[idx]; idx++ )
	{
	    if ( !strncmp(imageformatdescs[idx],filter.buf(),3) )
		ifmt = idx;
	}
    }

    return imageformats[ifmt] ? imageformats[ifmt] : imageformats[0];
}


void uiSaveImageDlg::getSettingsPar( PtrMan<IOPar>& ctiopar,
				     BufferString typenm )
{ ctiopar = settings_.subselect( typenm.buf() ); }


void uiSaveImageDlg::fillPar( IOPar& par, bool is2d )
{
    if ( !heightfld_ ) return;
    par.set( sKeyType(), is2d ? "2D" : "3D" );
    par.set( sKeyHeight(), heightfld_->box()->getFValue() );
    par.set( sKeyWidth(), widthfld_->box()->getFValue() );
    par.set( sKeyUnit(), unitfld_->text() );
    par.set( sKeyRes(), dpifld_->box()->getIntValue() );
    par.set( sKeyFileType(), getExtension() );
}


bool uiSaveImageDlg::usePar( const IOPar& par )
{
    if ( !heightfld_ ) return false;

    setNotifiers( false );

    BufferString res;
    bool ispixel = false;
    if ( par.get(sKeyUnit(),res) )
    {
	if ( res == "pixel" )
	    ispixel = true;
	else
	    unitfld_->setText( res );
    }

    float val;
    if ( par.get(sKeyHeight(),val) )
    {
	(ispixel ? pixheightfld_ : heightfld_)->box()->setValue( val );
	if ( !ispixel ) fldranges_.start = val;
    }
    if ( par.get(sKeyWidth(),val) )
    {
	(ispixel ? pixwidthfld_ : widthfld_)->box()->setValue( val );
	if ( !ispixel ) fldranges_.stop = val;
    }

    int dpi;
    if ( par.get(sKeyRes(),dpi) )
	dpifld_->box()->setValue( dpi );

    res.setEmpty();
    par.get( sKeyFileType(), res );

    int idx = 0;
    while ( imageformats[idx] )
    {
	if ( res != imageformats[idx] )
	{
	    idx++;
	    continue;
	}

	fileinputfld_->setSelectedFilter( imageformatdescs[idx] );
	fileinputfld_->setDefaultExtension( imageformats[idx] );
	break;
    }

    if ( ispixel )
	setSizeInPix( (int)sizepix_.width(), (int)sizepix_.height() );
    else
	updateSizes();

    setNotifiers( true );
    return true;
}


void uiSaveImageDlg::setSizeInPix( int width, int height )
{
    sizepix_.setWidth( mCast(float,width) );
    sizepix_.setHeight( mCast(float,height) );
    const float dpi = dpifld_->box()->getFValue();
    sPixels2Inch( sizepix_, sizeinch_, dpi );
    sInch2Cm( sizeinch_, sizecm_ );
    unitChg( 0 );
}


void uiSaveImageDlg::addPrintFmtFilters( BufferString& filters )
{
    filters += ";;";
    filters += imageformatdescs[sPDFfmtIdx];
    filters += ";;";
    filters += imageformatdescs[sPSfmtIdx];
    filters += ";;";
    filters += imageformatdescs[sEPSfmtIdx];
}


uiSaveWinImageDlg::uiSaveWinImageDlg( uiParent* p )
    : uiSaveImageDlg(p,true,false)
{
    enableSaveButton( uiStrings::sEmptyString() );
    screendpi_ = mCast(float,uiMain::getMinDPI());
    mDynamicCastGet(uiMainWin*,mw,p);
    aspectratio_ = 1.0f;
    if ( mw )
	aspectratio_ = mCast(float,mw->geometry().width())/
		       mCast(float,mw->geometry().height());
    createGeomInpFlds( cliboardselfld_ );
    if ( useparsfld_ ) useparsfld_->display( false, true );
    dpifld_->box()->setValue( screendpi_ );
    setFldVals( 0 );
    lockfld_->setChecked( true );
    lockfld_->setSensitive( false );
    updateFilter();
}


void uiSaveWinImageDlg::setFldVals( CallBacker* )
{
    mDynamicCastGet(uiMainWin*,mw,parent());
    if ( mw )
    {
	const int w = mw->geometry().width();
	const int h = mw->geometry().height();
	setSizeInPix( w, h );
    }
}


void uiSaveWinImageDlg::getSupportedFormats( const char** imagefrmt,
					     const char** frmtdesc,
					     BufferString& filters )
{
    BufferStringSet supportedformats;
    supportedImageFormats( supportedformats );
    int idy = 0;
    while ( imagefrmt[idy] )
    {
	if ( supportedformats.isPresent(imagefrmt[idy]) )
	{
	    if ( !filters.isEmpty() ) filters += ";;";
	    filters += frmtdesc[idy];
	}
	idy++;
    }

    uiSaveImageDlg::addPrintFmtFilters( filters );
}


bool uiSaveWinImageDlg::acceptOK( CallBacker* )
{
    mDynamicCastGet(uiMainWin*,mw,parent());
    if ( !mw ) return false;
    if ( cliboardselfld_->isChecked() )
    {
	mw->copyToClipBoard();
	return true;
    }

    if ( !filenameOK() ) return false;

    BufferString ext( getExtension() );
    if ( ext == "pdf" )
	mw->saveAsPDF( fileinputfld_->fileName(),(int)sizepix_.width(),
		       (int)sizepix_.height(),dpifld_->box()->getIntValue());
    else
	mw->saveImage( fileinputfld_->fileName(), (int)sizepix_.width(),
		       (int)sizepix_.height(), dpifld_->box()->getIntValue());
    return true;
}
