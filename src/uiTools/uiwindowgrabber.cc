/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          July 2008
________________________________________________________________________

-*/

#include "uiwindowgrabber.h"

#include "uicombobox.h"
#include "uifilesel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uislider.h"

#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "oddirs.h"
#include "settings.h"
#include "timer.h"
#include "mousecursor.h"
#include "od_helpids.h"

BufferString uiWindowGrabDlg::dirname_ = "";
static const char* wantedimageextensions[] =
{ "jpg", "png", "bmp", "ppm", "xpm", 0 };


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
	windowfld_->box()->setHSzPol( uiObject::Wide );
	windowfld_->box()->addItems( windownms );
    }

    if ( dirname_.isEmpty() )
	dirname_ = File::Path(GetDataDir()).add("Misc").fullPath();
    uiFileSel::Setup fssu( OD::GeneralContent );
    fssu.setForWrite().initialselectiondir( dirname_ )
		      .allowallextensions( false );
    inpfilefld_ = new uiFileSel( this, uiStrings::sFileName(), fssu );
    if ( windowfld_ )
	inpfilefld_->attach( alignedBelow, windowfld_ );
    updateFilter();

    qualityfld_ = new uiSlider( this,
	    uiSlider::Setup(tr("Image quality")).withedit(true),
	    "Quality slider" );
    qualityfld_->attach( alignedBelow, inpfilefld_ );
    qualityfld_->setInterval( StepInterval<float>(0,100,1) );
    qualityfld_->setValue(50);
    qualityfld_->attach( alignedBelow, inpfilefld_ );

    infofld_ = new uiLabel( this, tr("Arrange your windows before confirming"));
    infofld_->attach( alignedBelow, qualityfld_ );

    DBM().afterSurveyChange.notify( mCB(this,uiWindowGrabDlg,surveyChanged) );
}


void uiWindowGrabDlg::surveyChanged( CallBacker* )
{
    dirname_ = "";
}


void uiWindowGrabDlg::updateFilter()
{
    Settings& settings = Settings::fetch("snapshot");
    PtrMan<IOPar> iopar = settings.subselect( "3D" );
    BufferString deftype;
    if ( iopar )
	iopar->get( "File type", deftype );

    fileformats_.setEmpty();
    OD::GetSupportedImageFormats( fileformats_, false );
    for ( int ifmt=0; ifmt<fileformats_.size(); ifmt++ )
    {
	const File::Format fmt = fileformats_.format( ifmt );
	bool found = false;
	for ( int idx=0; wantedimageextensions[idx]; idx++ )
	{
	    if ( fmt.hasExtension(wantedimageextensions[idx]) )
		{ found = true; break ; }
	}
	if ( !found )
	{
	    fileformats_.removeFormat( ifmt );
	    ifmt--; continue;
	}
	if ( fmt.hasExtension(deftype) )
	    inpfilefld_->setup().defaultextension( deftype );
    }

    inpfilefld_->setup().formats( fileformats_ );
}


bool uiWindowGrabDlg::filenameOK() const
{
    BufferString filename = inpfilefld_->fileName();
    if ( filename.isEmpty() )
    {
	uiMSG().error( uiStrings::phrSelect(uiStrings::sFileName().toLower()) );
	return false;
    }

    if ( File::exists(filename) && !File::isWritable(filename) )
    {
	uiString msg = tr("The file %1 is not writable").arg(filename);
	uiMSG().error( msg );
	return false;
    }

    const BufferString ext( File::Path(filename).extension() );
    if ( ext.isEmpty() || !fileformats_.isPresent(ext) )
    {
	uiString msg = tr("The file %1 is not in a supported format")
			.arg(filename);
	uiMSG().error( msg );
	return false;
    }

    return true;
}


bool uiWindowGrabDlg::acceptOK()
{
    if ( !filenameOK() )
	return false;

    File::Path filepath( inpfilefld_->fileName() );
    dirname_ = filepath.pathOnly();
    filename_ = filepath.fullPath();
    return true;
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
