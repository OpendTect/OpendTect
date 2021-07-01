/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/05/2000
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "odwindow.h"

#include "uiclipboard.h"
#include "uimain.h"
#include "uimenu.h"
#include "uipixmap.h"
#include "uistatusbar.h"
#include "uitoolbar.h"

#include "file.h"
#include "msgh.h"
#include "q_uiimpl.h"
#include "texttranslation.h"

#include <QAbstractButton>
#include <QApplication>
#include <QColorDialog>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFontDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPainter>
#include <QtPlatformHeaders/QWindowsWindowFunctions>
#include <QPrinter>
#include <QScreen>

mUseQtnamespace

static uiMainWin*		programmedactivewin_ = nullptr;

uiMainWin::uiMainWin( uiParent* p, const uiMainWin::Setup& setup )
    : uiParent(toString(setup.caption_),0)
    , parent_(p)
    , windowShown(this)
    , windowHidden(this)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , caption_(setup.caption_)
    , languagechangecount_( TrMgr().changeCount() )
{
    const BufferString bodynm( toString(setup.caption_) );
    body_ = new uiMainWinBody( *this, p, bodynm, setup.modal_ );
    setBody( body_ );
    body_->construct( setup.nrstatusflds_, setup.withmenubar_ );
    body_->setWindowIconText( setup.caption_.isEmpty()
		? QString("OpendTect") : toQString(setup.caption_) );
    body_->setAttribute( Qt::WA_DeleteOnClose, setup.deleteonclose_ );

    finishConstruction();
}


uiMainWin::uiMainWin( uiParent* parnt, const uiString& cpt,
		      int nrstatusflds, bool withmenubar, bool modal )
    : uiParent(toString(cpt),0)
    , parent_(parnt)
    , popuparea_(Middle)
    , windowShown(this)
    , windowHidden(this)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , caption_(cpt)
    , languagechangecount_( TrMgr().changeCount() )
{
    body_ = new uiMainWinBody( *this, parnt, toString(caption_), modal );
    setBody( body_ );
    body_->construct( nrstatusflds, withmenubar );
    body_->setWindowIconText( caption_.isEmpty()
			     ? QString("OpendTect") : toQString(caption_) );
    finishConstruction();
}


uiMainWin::uiMainWin( const uiString& captn, uiParent* parnt )
    : uiParent(toString(captn),0)
    , parent_(parnt)
    , popuparea_(Middle)
    , windowShown(this)
    , windowHidden(this)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , caption_(captn)
{
    // no finishConstruction() - this constructor is for subclasses that need
    // to call it explicitly at end-of-constructor
}


uiMainWin::~uiMainWin()
{
    detachAllNotifiers();

    if ( body_ )
 	body_->removeWindow( *this );

    if ( !body_->deletefrombody_ )
    {
	body_->deletefromod_ = true;
	delete body_;
    }

    if ( programmedactivewin_ == this )
	programmedactivewin_ = parent() ? parent()->mainwin() : nullptr;

    delete afterpopuptimer_;
}


void uiMainWin::finishConstruction()
{
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoardCB) );
    mAttachCB( TrMgr().languageChange, uiMainWin::languageChangeCB );
    if ( body_ )
	body_->addMainWindow( *this );
}


QWidget* uiMainWin::getWidget( int )
{ return body_; }


uiStatusBar* uiMainWin::statusBar()		{ return body_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return body_->uimenubar(); }


void uiMainWin::show()
{
    body_->go();
}


void uiMainWin::close()				{ body_->close(); }
void uiMainWin::reDraw(bool deep)		{ body_->reDraw(deep); }
bool uiMainWin::poppedUp() const		{ return body_->poppedUp(); }
bool uiMainWin::resetPopupTimerIfNotPoppedUp()	{ return body_->
					    resetPopupTimerIfNotPoppedUp(); }
bool uiMainWin::finalised() const		{ return body_->finalised(); }
void uiMainWin::setExitAppOnClose( bool yn )	{ body_->exitapponclose_ = yn; }
void uiMainWin::showMaximized()			{ body_->showMaximized(); }
void uiMainWin::showMinimized()			{ body_->showMinimized(); }
void uiMainWin::showNormal()			{ body_->showNormal(); }
bool uiMainWin::isMaximized() const		{ return body_->isMaximized(); }
bool uiMainWin::isMinimized() const		{ return body_->isMinimized(); }
bool uiMainWin::isHidden() const		{ return body_->isHidden(); }
bool uiMainWin::isModal() const			{ return body_->isModal(); }
void uiMainWin::setForceFinalise( bool yn )	{ body_->force_finalise_ = yn; }

void uiMainWin::forceClose()
{
    if ( !isModal() )
	setDeleteOnClose( true );
    close();
}


void uiMainWin::setCaption( const uiString& txt )
{
    caption_ = txt;
    updateCaption();
}


void uiMainWin::updateCaption()
{
    uniquecaption_ = uniqueWinTitle(caption_,body_,0);
    body_->setWindowTitle( toQString(uniquecaption_) );
}


const uiString& uiMainWin::caption( bool unique ) const
{
    return unique ? uniquecaption_ : caption_;
}


void uiMainWin::setDeleteOnClose( bool yn )
{ body_->setAttribute( Qt::WA_DeleteOnClose, yn ); }


void uiMainWin::removeDockWindow( uiDockWin* dwin )
{ body_->removeDockWin( dwin ); }


void uiMainWin::addDockWindow( uiDockWin& dwin, Dock d )
{ body_->addDockWin( dwin, d ); }


void uiMainWin::addToolBar( uiToolBar* tb )
{ body_->addToolBar( tb ); }

uiToolBar* uiMainWin::findToolBar( const char* nm )
{ return body_->findToolBar( nm ); }

uiToolBar* uiMainWin::removeToolBar( uiToolBar* tb )
{ return body_->removeToolBar( tb ); }


void uiMainWin::addToolBarBreak()
{ body_->addToolBarBreak(); }


uiMenu& uiMainWin::getToolbarsMenu() const
{ return body_->getToolbarsMenu(); }


const ObjectSet<uiToolBar>& uiMainWin::toolBars() const
{ return body_->toolBars(); }


const ObjectSet<uiDockWin>& uiMainWin::dockWins() const
{ return body_->dockWins(); }


uiGroup* uiMainWin::topGroup()
{ return body_->uiCentralWidg(); }


void uiMainWin::setShrinkAllowed(bool yn)
    { if ( topGroup() ) topGroup()->setShrinkAllowed(yn); }


bool uiMainWin::shrinkAllowed()
    { return topGroup() ? topGroup()->shrinkAllowed() : false; }


uiObject* uiMainWin::mainobject()
    { return body_->uiCentralWidg()->mainObject(); }


void uiMainWin::toStatusBar( const uiString& txt, int fldidx, int msecs )
{
    uiStatusBar* sb = statusBar();
    if ( sb )
	sb->message( txt, fldidx, msecs );
    else if ( !txt.isEmpty() )
	UsrMsg( txt );
}


void uiMainWin::setSensitive( bool yn )
{
    if ( menuBar() ) menuBar()->setSensitive( yn );
    body_->setEnabled( yn );
}


uiMainWin* uiMainWin::gtUiWinIfIsBdy(QWidget* mwimpl)
{
    if ( !mwimpl ) return 0;

    uiMainWinBody* _mwb = dynamic_cast<uiMainWinBody*>( mwimpl );
    if ( !_mwb ) return 0;

    return &_mwb->handle_;
}


void uiMainWin::setCornerPos( int x, int y )
{ body_->move( x, y ); }


uiRect uiMainWin::geometry( bool frame ) const
{
    // Workaround for Qt-bug: top left of area sometimes translates to origin!
    QRect qarea = body_->geometry();
    QRect qframe = body_->frameGeometry();
    QPoint correction = body_->mapToGlobal(QPoint(0,0)) - qarea.topLeft();
    qframe.translate( correction );
    qarea.translate( correction );
    QRect qrect = frame ? qframe : qarea;

    //QRect qrect = frame ? body_->frameGeometry() : body_->geometry();
    uiRect rect( qrect.left(), qrect.top(), qrect.right(), qrect.bottom() );
    return rect;
}


bool uiMainWin::doSetWindowFlags( od_uint32 todoflagi, bool setyn )
{
    const Qt::WindowFlags todoflag( todoflagi );
    const Qt::WindowFlags flags = body_->windowFlags();
    if ( ( setyn &&  (flags & todoflag)) ||
         (!setyn && !(flags & todoflag)) )
	return false;

    if ( !isMinimized() && !isHidden() )
	pErrMsg( "Setting window flags on a displayed widget "
		 "hides it (see Qt doc)" );

    const bool isfinalised = finalised();
    body_->doSetWindowFlags( todoflag, setyn );
    if ( isfinalised )
	Threads::sleep( 0.3 );
    return true;
}


bool uiMainWin::showMinMaxButtons( bool yn )
{ return doSetWindowFlags( Qt::WindowMinMaxButtonsHint, yn ); }

bool uiMainWin::showAlwaysOnTop( bool yn )
{ return doSetWindowFlags( Qt::WindowStaysOnTopHint, yn ); }


void uiMainWin::showAndActivate()
{
    if ( isMinimized() || isHidden() )
	showNormal();
    activate();
}


void uiMainWin::setActivateOnFirstShow( bool yn )
{
    setActivateBehaviour( yn ? OD::AlwaysActivateWindow
			     : OD::DefaultActivateWindow );
}


static OD::WindowActivationBehavior activateAct = OD::DefaultActivateWindow;

void uiMainWin::setActivateBehaviour(
		    OD::WindowActivationBehavior activateact )
{
    if ( !__iswin__ )
	return;

    activateAct = activateact;
#if QT_VERSION >= QT_VERSION_CHECK(5,7,0)
    const bool alwayshow = activateAct == OD::AlwaysActivateWindow;
    QWindowsWindowFunctions::setWindowActivationBehavior(
	  alwayshow ? QWindowsWindowFunctions::AlwaysActivateWindow
		    : QWindowsWindowFunctions::DefaultActivateWindow );
#endif
}


OD::WindowActivationBehavior uiMainWin::getActivateBehaviour()
{
    return activateAct;
}


void uiMainWin::activate()
{
    if ( !finalised() )
	return;

    setActivateOnFirstShow();
    body_->activateWindow();
}


void uiMainWin::setIcon( const uiPixmap& pm )
{ body_->setWindowIcon( *pm.qpixmap() ); }

void uiMainWin::setIcon( const char* iconid )
{ setIcon( uiPixmap(iconid) ); }

void uiMainWin::setIconText( const uiString& txt)
{ body_->setWindowIconText( toQString(txt) ); }

void uiMainWin::saveSettings()
{ body_->saveSettings(); }

void uiMainWin::readSettings()
{ body_->readSettings(); }

void uiMainWin::raise()
{ body_->raise(); }


void uiMainWin::programActiveWindow( uiMainWin* mw )
{ programmedactivewin_ = mw; }


uiMainWin* uiMainWin::programmedActiveWindow()
{ return programmedactivewin_; }


void uiMainWin::runScript( const char* filename )
{
    scripttorun_ = filename;
    runScriptRequest.trigger();
}


const char* uiMainWin::getScriptToRun() const
{ return scripttorun_; }


uiMainWin* uiMainWin::activeWindow()
{
    if ( programmedactivewin_ )
	return programmedactivewin_;

    QWidget* _aw = qApp->activeWindow();
    if ( !_aw )		return 0;

    uiMainWinBody* _awb = dynamic_cast<uiMainWinBody*>(_aw);
    if ( !_awb )	return 0;

    return &_awb->handle_;
}


uiMainWin::ActModalTyp uiMainWin::activeModalType()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )					return None;

    if ( dynamic_cast<uiMainWinBody*>(amw) )	return Main;
    if ( dynamic_cast<QMessageBox*>(amw) )	return Message;
    if ( dynamic_cast<QFileDialog*>(amw) )	return File;
    if ( dynamic_cast<QColorDialog*>(amw) )	return Colour;
    if ( dynamic_cast<QFontDialog*>(amw) )	return Font;

    return Unknown;
}


uiMainWin* uiMainWin::activeModalWindow()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )	return 0;

    uiMainWinBody* mwb = dynamic_cast<uiMainWinBody*>( amw );
    if ( !mwb )	return 0;

    return &mwb->handle_;
}


BufferString uiMainWin::activeModalQDlgTitle()
{
    QWidget* amw = qApp->activeModalWidget();
    return amw ? BufferString( amw->windowTitle() ) : BufferString();
}


static QMessageBox::StandardButton getStandardButton( const QMessageBox* qmb,
						      int buttonnr )
{
    int stdbutcount = 0;

    for ( unsigned int idx=QMessageBox::Ok;
	  qmb && idx<=QMessageBox::RestoreDefaults; idx+=idx )
    {
	if ( !qmb->button((QMessageBox::StandardButton) idx) )
	    continue;

	if ( stdbutcount == buttonnr )
	    return (QMessageBox::StandardButton) idx;

	stdbutcount++;
    }

    return QMessageBox::NoButton;
}


BufferString uiMainWin::activeModalQDlgButTxt( int buttonnr )
{
    const ActModalTyp typ = activeModalType();
    QWidget* amw = qApp->activeModalWidget();

    BufferString ret;
    if ( typ == Message )
    {
	const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw );

	const QMessageBox::StandardButton stdbut =
					getStandardButton( qmb, buttonnr );
	if ( stdbut )
	    ret = qmb->button(stdbut)->text();
	    // TODO: get original text if button text is translation
    }
    else if ( typ==Colour || typ==Font || typ==File )
    {
	if ( buttonnr == 0 )
	    ret = "Cancel";
	else if ( buttonnr == 1 )
	    ret = typ==File ? "Ok" : "OK";
    }

    return ret;
}


int uiMainWin::activeModalQDlgRetVal( int buttonnr )
{
    QWidget* amw = qApp->activeModalWidget();
    const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw );
    const QMessageBox::StandardButton stdbut =
					getStandardButton( qmb, buttonnr );

    return stdbut ? ((int) stdbut) : buttonnr;
}


void uiMainWin::closeActiveModalQDlg( int retval )
{
    if ( activeModalWindow() )
	return;

    QWidget* _amw = qApp->activeModalWidget();
    if ( !_amw )
	return;

    QDialog* _qdlg = dynamic_cast<QDialog*>(_amw);
    if ( !_qdlg )
	return;

    _qdlg->done( retval );
}


void uiMainWin::getTopLevelWindows( ObjectSet<uiMainWin>& windowlist,
				    bool visibleonly )
{
    uiMainWinBody::getTopLevelWindows( windowlist, visibleonly );
}


void uiMainWin::getModalSignatures( BufferStringSet& signatures )
{
    signatures.erase();
    QWidgetList toplevelwigs = qApp->topLevelWidgets();

    for ( int idx=0; idx<toplevelwigs.count(); idx++ )
    {
	const QWidget* qw = toplevelwigs.at( idx );
	if ( qw->isWindow() && !qw->isHidden() && qw->isModal() )
	{
	    BufferString qwptrstr( 256, true );
#ifdef __win__
	    sprintf_s( qwptrstr.getCStr(), qwptrstr.bufSize(), "%p", qw );
#else
	    sprintf( qwptrstr.getCStr(), "%p", qw );
#endif
	    signatures.add( qwptrstr );
	}
    }
}


uiString uiMainWin::uniqueWinTitle( const uiString& txt,
				    QWidget* forwindow,
				    BufferString* outputaddendum )
{
    const QWidgetList toplevelwigs = qApp->topLevelWidgets();

    uiString res;
    for ( int count=1; true; count++ )
    {
	bool unique = true;
	uiString beginning = txt.isEmpty() ? tr("<no title>") : txt;

	if ( count>1 )
	{
	    res = toUiString( "%1 {%2}").arg( beginning )
					.arg( toUiString(count) );
	    BufferString addendum( "  {", toString(count), "}" );
	    if ( outputaddendum ) *outputaddendum = addendum;
	}
	else
	{
	    res = beginning;
	}

	QString wintitle = toQString( res );

	for ( int idx=0; idx<toplevelwigs.count(); idx++ )
	{
	    const QWidget* qw = toplevelwigs.at( idx );
	    if ( !qw->isWindow() || qw->isHidden() || qw==forwindow )
		continue;

	    if ( qw->windowTitle() == wintitle )
	    {
		unique = false;
		break;
	    }
	}

	if ( unique ) break;
    }

    return res;
}


std::string OD_Win_GetSnapShotFile(const std::string&);

bool uiMainWin::grab( const char* filenm, int zoom,
		      const char* format, int quality ) const
{
#ifdef __win__

    std::string snapshotfile = OD_Win_GetSnapShotFile( filenm );
    if ( snapshotfile.empty() )
	return false;

    QPixmap desktopsnapshot;

    if ( !desktopsnapshot.load( snapshotfile.c_str() ) )
	{ ErrMsg( "Generated GDI+ image does not load in Qt" ); return false; }

    File::remove( snapshotfile.c_str() );

#else

    QScreen* qscreen = body_ ? body_->screen() : uiMainWinBody::primaryScreen();
    if ( !qscreen ) return false;
    const WId desktopwinid = QApplication::desktop()->winId();
    QPixmap desktopsnapshot = qscreen->grabWindow( desktopwinid );

#endif

    if ( zoom > 0 )
    {
	QWidget* qwin = qApp->activeModalWidget();
	if ( !qwin || zoom==1 )
	    qwin = body_;

#ifdef __win__

	RECT rect = {};
	GetWindowRect( (HWND)qwin->winId() , &rect );
	const int width  = rect.right - rect.left;
	const int height = rect.bottom - rect.top;

#else

	const int width = qwin->frameGeometry().width();
	/*on windows, it gets width till end of monitor and not entire widget*/
	const int height = qwin->frameGeometry().height();

#endif
	desktopsnapshot = desktopsnapshot.copy( qwin->x(), qwin->y(),
						width, height );
    }

    return desktopsnapshot.save( QString(filenm), format, quality );
}


bool uiMainWin::grabScreen( const char* filenm, const char* format, int quality,
			    int screenidx )
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    if ( screens.isEmpty() ) return false;

    const int nrscreens = screens.size();
    QScreen* qscreen = 0;
    if ( screenidx==-1 )
	qscreen = QGuiApplication::primaryScreen();
    else if ( screenidx>=0 && screenidx<nrscreens )
	qscreen = screens.at( screenidx );
    else
	qscreen = screens.first();

    if ( !qscreen ) return false;

    const QRect geom = qscreen->geometry();
    QPixmap snapshot = qscreen->grabWindow( 0, geom.left(), geom.top(),
					    geom.width(), geom.height() );
    return snapshot.save( QString(filenm), format, quality );
}


void uiMainWin::activateInGUIThread( const CallBack& cb, bool busywait )
{ body_->activateInGUIThread( cb, busywait ); }


void uiMainWin::translateText()
{
    uiParent::translateText();

    for ( int idx=0; idx<body_->toolbars_.size(); idx++ )
	body_->toolbars_[idx]->translateText();

    //Don't know if anything special needs to be done here.
}


void uiMainWin::copyToClipBoardCB( CallBacker* )
{
    copyToClipBoard();
}


void uiMainWin::languageChangeCB( CallBacker* )
{
    if ( languagechangecount_<TrMgr().changeCount() )
    {
	translateText();
	languagechangecount_ = TrMgr().changeCount();
    }
}


void uiMainWin::aftPopupCB( CallBacker* )
{
    afterPopup.trigger();
}



class ImageSaver : public CallBacker
{ mODTextTranslationClass(ImageSaver)
public:

ImageSaver()
{
    timer_.tick.notify( mCB(this,ImageSaver,shootImageCB) );
}


void setImageProp( WId qwid, int w, int h, int r )
{
    qwinid_ = qwid;
    width_ = w;
    height_ = h;
    res_ = r;
    copytoclipboard_ = true;
    timer_.start( 500, true );
}


void setImageProp( WId qwid, const char* fnm, int w, int h, int r )
{
    qwinid_ = qwid;
    fname_ = fnm;
    width_ = w;
    height_ = h;
    res_ = r;
    copytoclipboard_ = false;
    timer_.start( 500, true );
}


protected:
void shootImageCB( CallBacker* )
{
    QScreen* qscreen = uiMainWinBody::primaryScreen();
    if ( !qscreen ) return;
    const QPixmap snapshot = qscreen->grabWindow( qwinid_ );

    QImage image = snapshot.toImage();
    image = image.scaledToWidth( width_ );
    image = image.scaledToHeight( height_ );
    image.setDotsPerMeterX( (int)(res_/0.0254) );
    image.setDotsPerMeterY( (int)(res_/0.0254) );
    if ( copytoclipboard_ )
	uiClipboard::setImage( image );
    else
	image.save( fname_ );

    timer_.stop();
}

    int		width_;
    int		height_;
    int		res_;
    bool	copytoclipboard_;
    QString	fname_;
    WId		qwinid_;
    Timer	timer_;
};


void uiMainWin::copyToClipBoard()
{
    QWidget* qwin = getWidget(0);
    if ( !qwin )
	qwin = body_;
    WId wid = qwin->winId();
    const int width = qwin->frameGeometry().width();
    const int height = qwin->frameGeometry().height();
    mDefineStaticLocalObject( ImageSaver, imagesaver, );
    imagesaver.setImageProp( wid, width, height, uiMain::getMinDPI() );
}


void uiMainWin::saveImage( const char* fnm, int width, int height, int res )
{
    QWidget* qwin = getWidget(0);
    if ( !qwin )
	qwin = body_;
    WId wid = qwin->winId();
    mDefineStaticLocalObject( ImageSaver, imagesaver, );
    imagesaver.setImageProp( wid, fnm, width, height, res );
}


void uiMainWin::saveAsPDF_PS( const char* filename, bool, int w,
				    int h, int res )
{
    QString fileName( filename );
    auto* pdfprinter = new QPrinter();
    pdfprinter->setOutputFormat( QPrinter::PdfFormat );
    const QPageSize pgsz( QSizeF(w,h), QPageSize::Point );
    pdfprinter->setPageSize( pgsz );
    pdfprinter->setFullPage( false );
    pdfprinter->setOutputFileName( filename );
    pdfprinter->setResolution( res );

    auto* pdfpainter = new QPainter();
    pdfpainter->begin( pdfprinter );
    QWidget* qwin = getWidget(0);
    const QRect qrec =
	pdfprinter->pageLayout().paintRectPixels( pdfprinter->resolution() );
    qwin->render( pdfpainter, qrec.topLeft(), qwin->rect() );
    pdfpainter->end();
    delete pdfpainter;
    delete pdfprinter;
}
