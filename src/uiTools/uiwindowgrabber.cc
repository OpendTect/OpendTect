/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          July 2008
________________________________________________________________________

-*/

#include "uiwindowgrabber.h"

#include "uicombobox.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uislider.h"

#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "settings.h"
#include "timer.h"
#include "od_helpids.h"

BufferString uiWindowGrabDlg::dirname_ = "";

static const char* imageformats[] =
{ "jpg", "png", "tiff", "webp", "bmp", "ppm", "xpm", 0 };

static const char* filters[] =
{
    "JPEG (*.jpg *.jpeg)",
    "PNG (*.png)",
    "TIFF (*.tif *.tiff)",
    "WebP (*.webp)",
    "Windows Bitmap (*.bmp)",
    "PPM (*.ppm)",
    "XPM (*.xpm)",
    0
};



uiWindowGrabDlg::uiWindowGrabDlg( uiParent* p, bool desktop )
    : uiDialog(p,uiDialog::Setup( (desktop ? tr("Desktop grabber") :
					     tr("Window grabber")),
				  (desktop ? tr("Specify image file") :
					   tr("Specify window and image file")),
		 desktop ? mODHelpKey(mWindowGrabDlgDesktopHelpID) :
			   mODHelpKey(mWindowGrabDlgWindowHelpID) ) )
    , windowfld_(0)
{
    getTopLevelWindows( windowlist_ );
    if ( !desktop && !windowlist_.isEmpty() )
    {
	uiStringSet windownms;
	for ( int idx=0; idx<windowlist_.size(); idx++ )
	    windownms.add( windowlist_[idx]->caption(true) );

	windowfld_ = new uiLabeledComboBox( this, tr("Grab window") );
	windowfld_->box()->addItems( windownms );
	windowfld_->box()->setHSzPol( uiObject::Wide );
    }

    if ( dirname_.isEmpty() )
	dirname_ = FilePath(GetDataDir()).add("Misc").fullPath();
    fileinputfld_ = new uiFileInput( this, mJoinUiStrs(sFile(),sName()),
				    uiFileInput::Setup(uiFileDialog::Gen)
				    .forread(false)
				    .defseldir(dirname_)
				    .directories(false)
				    .allowallextensions(false) );
    fileinputfld_->valuechanged.notify( mCB(this,uiWindowGrabDlg,fileSel) );
    if ( windowfld_ )
	fileinputfld_->attach( alignedBelow, windowfld_ );
    updateFilter();

    qualityfld_ = new uiSlider( this,
	    uiSlider::Setup(tr("Image quality")).withedit(true),
	    "Quality slider" );
    qualityfld_->attach( alignedBelow, fileinputfld_ );
    qualityfld_->setInterval( StepInterval<float>(0,100,1) );
    qualityfld_->setValue(50);
    qualityfld_->attach( alignedBelow, fileinputfld_ );

    infofld_ = new uiLabel( this, tr("Arrange your windows before confirming"));
    infofld_->attach( alignedBelow, qualityfld_ );

    IOM().afterSurveyChange.notify( mCB(this,uiWindowGrabDlg,surveyChanged) );
}


void uiWindowGrabDlg::surveyChanged( CallBacker* )
{
    dirname_ = "";
}


void uiWindowGrabDlg::updateFilter()
{
    Settings& settings = Settings::fetch("snapshot");
    PtrMan<IOPar> iopar = settings.subselect( "3D" );
    BufferString deftype = imageformats[0];
    if ( iopar ) iopar->get( "File type", deftype );

    BufferStringSet supportedformats;
    supportedImageFormats( supportedformats );
    int idx = 0;
    while ( imageformats[idx] )
    {
	if ( deftype != imageformats[idx] )
	{
	    idx++;
	    continue;
	}

	if ( supportedformats.isPresent(imageformats[idx]) )
	    fileinputfld_->setSelectedFilter( filters[idx] );
	break;
    }

    BufferString filter;
    idx = 0;
    while ( imageformats[idx] )
    {
	const int idy = supportedformats.indexOf( imageformats[idx] );
	if ( idy>=0 )
	{
	    if ( !filter.isEmpty() ) filter += ";;";
	    filter += filters[idx];
	}
	idx++;
    }

    fileinputfld_->setFilter( filter );
}


void uiWindowGrabDlg::fileSel( CallBacker* )
{
    BufferString filename = fileinputfld_->fileName();
    addFileExtension( filename );
    fileinputfld_->setFileName( filename );
}


void uiWindowGrabDlg::addFileExtension( BufferString& filename )
{
    FilePath fp( filename.buf() );
    fp.setExtension( getExtension() );
    filename = fp.fullPath();
}


bool uiWindowGrabDlg::filenameOK() const
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


bool uiWindowGrabDlg::acceptOK( CallBacker* )
{
    if ( !filenameOK() )
	return false;

    FilePath filepath( fileinputfld_->fileName() );
    dirname_ = filepath.pathOnly();
    filename_ = filepath.fullPath();
    return true;
}


const char* uiWindowGrabDlg::getExtension() const
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
	const FixedString selectedfilter = fileinputfld_->selectedFilter();
	for ( ifmt=0; filters[ifmt]; ifmt++ )
	{
	    if ( selectedfilter == filters[ifmt] )
		break;
	}
    }

    return imageformats[ifmt] ? imageformats[ifmt] : imageformats[0];
}


uiMainWin* uiWindowGrabDlg::getWindow() const
{
    const int idx = windowfld_ ? windowfld_->box()->currentItem() : 0;
    return const_cast<uiMainWin*>( windowlist_[idx] );
};


int uiWindowGrabDlg::getQuality() const
{ return qualityfld_->getIntValue(); }


uiWindowGrabber::uiWindowGrabber( uiParent* p )
    : parent_(p)
    , desktop_(false)
    , grabwin_(0)
    , quality_(50)
    , tmr_(new Timer())
{
    tmr_->tick.notify( mCB(this,uiWindowGrabber,actCB) );
}


uiWindowGrabber::~uiWindowGrabber()
{
    delete tmr_;
}


bool uiWindowGrabber::go()
{
    if ( tmr_->isActive() )
	return false;

    uiWindowGrabDlg dlg( parent_, desktop_ );
    if ( !dlg.go() )
	return false;

    grabwin_ = dlg.getWindow();
    filename_ = dlg.getFilename();
    quality_ = dlg.getQuality();

    MouseCursorManager::setOverride( MouseCursor::Wait );

    // give window manager chance to remove the uiWindowGrabDlg from screen
    tmr_->start( 1000, true );

    return true;
}


void uiWindowGrabber::actCB( CallBacker* )
{
    const int zoom = desktop_ ? 0 : 1;
    grabwin_->grab( filename_, zoom, 0, quality_ );

    MouseCursorManager::restoreOverride();
}
