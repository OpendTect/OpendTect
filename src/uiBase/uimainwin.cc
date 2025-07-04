/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "odwindow.h"

#include "uiclipboard.h"
#include "uidockwin.h"
#include "uimain.h"
#include "uimenu.h"
#include "uipixmap.h"
#include "uistatusbar.h"
#include "uitoolbar.h"

#include "msgh.h"
#include "q_uiimpl.h"
#include "texttranslator.h"

#include <QAbstractButton>
#include <QApplication>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPainter>
#include <QScreen>
#ifdef __win__
# if QT_VERSION >= QT_VERSION_CHECK(5,7,0) && \
     QT_VERSION < QT_VERSION_CHECK(6,0,0)
# include <QtPlatformHeaders/QWindowsWindowFunctions>
#endif
#endif

mUseQtnamespace

static uiMainWin*		programmedactivewin_ = nullptr;

uiMainWin::uiMainWin( uiParent* p, const uiMainWin::Setup& setup )
    : uiParent(toString(setup.caption_),nullptr)
    , windowShown(this)
    , windowHidden(this)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , parent_(p)
    , popuparea_(Middle)
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
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoardCB) );
}


uiMainWin::uiMainWin( uiParent* parnt, const uiString& cpt,
		      int nrstatusflds, bool withmenubar, bool modal )
    : uiParent(toString(cpt),nullptr)
    , windowShown(this)
    , windowHidden(this)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , parent_(parnt)
    , popuparea_(Middle)
    , caption_(cpt)
    , languagechangecount_( TrMgr().changeCount() )
{
    body_ = new uiMainWinBody( *this, parnt, toString(caption_), modal );
    setBody( body_ );
    body_->construct( nrstatusflds, withmenubar );
    body_->setWindowIconText( caption_.isEmpty()
			     ? QString("OpendTect") : toQString(caption_) );
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoardCB) );

    mAttachCB( TrMgr().languageChange, uiMainWin::languageChangeCB );
}


uiMainWin::uiMainWin( const uiString& captn, uiParent* parnt )
    : uiParent(toString(captn),nullptr)
    , windowShown(this)
    , windowHidden(this)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , parent_(parnt)
    , popuparea_(Middle)
    , caption_(captn)
    , languagechangecount_( TrMgr().changeCount() )
{
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoardCB) );

    mAttachCB( TrMgr().languageChange, uiMainWin::languageChangeCB );
}


uiMainWin::~uiMainWin()
{
    detachAllNotifiers();

    if ( !body_->deletefrombody_ )
    {
	body_->deletefromod_ = true;
	delete body_;
    }

    if ( programmedactivewin_ == this )
	programmedactivewin_ = parent() ? parent()->mainwin() : nullptr;

    delete afterpopuptimer_;
}


QWidget* uiMainWin::qWidget() const
{ return body_; }


QScreen* uiMainWin::qScreen() const
{
    QScreen* qscreen = uiMainWinBody::primaryScreen();
    if ( body_ )
    {
	const QWidget* centralwidget = body_->centralWidget();
	qscreen = centralwidget ? centralwidget->screen() : body_->screen();
    }
    return qscreen;
}


uiStatusBar* uiMainWin::statusBar()		{ return body_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return body_->uimenubar(); }


void uiMainWin::show()
{
    body_->go();
}


void uiMainWin::close()				{ body_->close(); }
void uiMainWin::reDraw(bool deep)		{ body_->reDraw(deep); }
bool uiMainWin::poppedUp() const		{ return body_->poppedUp(); }
bool uiMainWin::touch()				{ return body_->touch(); }
bool uiMainWin::finalized() const		{ return body_->finalized(); }
void uiMainWin::setExitAppOnClose( bool yn )	{ body_->exitapponclose_ = yn; }
void uiMainWin::showMaximized()			{ body_->showMaximized(); }
void uiMainWin::showMinimized()			{ body_->showMinimized(); }
void uiMainWin::showNormal()			{ body_->showNormal(); }
bool uiMainWin::isMaximized() const		{ return body_->isMaximized(); }
bool uiMainWin::isMinimized() const		{ return body_->isMinimized(); }
bool uiMainWin::isHidden() const		{ return body_->isHidden(); }
bool uiMainWin::isModal() const			{ return body_->isModal(); }
void uiMainWin::setForceFinalize( bool yn )	{ body_->force_finalize_ = yn; }


void uiMainWin::setContentsMargins( int left, int top, int right, int bottom )
{
    body_->setContentsMargins( left, top, right, bottom );
}


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


bool uiMainWin::isDeleteOnClose() const
{ return body_->isDeleteOnClose(); }


void uiMainWin::removeDockWindow( uiDockWin* dwin )
{ body_->removeDockWin( dwin ); }


void uiMainWin::addDockWindow( uiDockWin& dwin, Dock d )
{ body_->addDockWin( dwin, d ); }


void uiMainWin::tabifyDockWindow( uiDockWin& first, uiDockWin& second )
{
    body_->tabifyDockWidget( first.qwidget(), second.qwidget() );
}


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
	UsrMsg(txt.getFullString());
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


void uiMainWin::setGeometry( const uiRect& rect )
{
    body_->setGeometry( rect.left(), rect.top(), rect.width(), rect.height() );
}


void uiMainWin::adjustSize()
{
    body_->adjustSize();
}


void uiMainWin::setSizeRelativeToScreen( float width, float height )
{
    if ( width<0.05 || width>1 )
	width = 1;
    if ( height<0.05 || height>1 )
	height = 1;

    uiSize screensz = uiMain::instance().desktopSize();
    if ( mIsUdf(screensz.width()) )
	screensz = uiMain::instance().getScreenSize( 0, true );

    if ( !mIsUdf(screensz.width()) )
	setGeometry( uiRect(0,0,width*screensz.width(),
			    height*screensz.height()) );
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

    const bool isfinalized = finalized();
    body_->doSetWindowFlags( todoflag, setyn );
    if ( isfinalized )
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
    setActivateBehavior( yn ? OD::AlwaysActivateWindow
			     : OD::DefaultActivateWindow );
}


static OD::WindowActivationBehavior activateAct = OD::DefaultActivateWindow;

void uiMainWin::setActivateBehavior(
		    OD::WindowActivationBehavior activateact )
{
#ifdef __win__
    activateAct = activateact;
# if QT_VERSION >= QT_VERSION_CHECK(5,7,0) && \
     QT_VERSION < QT_VERSION_CHECK(6,0,0)
    const bool alwayshow = activateAct == OD::AlwaysActivateWindow;
    QWindowsWindowFunctions::setWindowActivationBehavior(
	  alwayshow ? QWindowsWindowFunctions::AlwaysActivateWindow
		    : QWindowsWindowFunctions::DefaultActivateWindow );
# endif
#endif
}


OD::WindowActivationBehavior uiMainWin::getActivateBehavior()
{
    return activateAct;
}


void uiMainWin::activate()
{
    if ( !finalized() )
	return;

    setActivateOnFirstShow();
    body_->activateWindow();
}


void uiMainWin::setIcon( const uiPixmap& pm )
{ body_->setWindowIcon( *pm.qpixmap() ); }

void uiMainWin::setIconText( const uiString& txt)
{ body_->setWindowIconText( toQString(txt) ); }

void uiMainWin::saveSettings()
{ body_->saveSettings(); }

void uiMainWin::readSettings()
{ body_->readSettings(); }


void uiMainWin::restoreDefaultState()
{ body_->restoreDefaultState(); }

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
    if ( dynamic_cast<QColorDialog*>(amw) )	return Color;
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


const char* uiMainWin::activeModalQDlgTitle()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )
	return 0;

    mDeclStaticString( title );
    title = amw->windowTitle();
    return title;
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


const char* uiMainWin::activeModalQDlgButTxt( int buttonnr )
{
    const ActModalTyp typ = activeModalType();
    QWidget* amw = qApp->activeModalWidget();

    if ( typ == Message )
    {
	const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw );
	mDeclStaticString( buttext );

	const QMessageBox::StandardButton stdbut =
					getStandardButton( qmb, buttonnr );
	if ( stdbut )
	    // TODO: get original text if button text is translation
	    buttext = qmb->button(stdbut)->text();
	else
	    buttext = "";

	return buttext;
    }

    if ( typ==Color || typ==Font || typ==File )
    {
	if ( buttonnr == 0 ) return "Cancel";
	if ( buttonnr == 1 ) return typ==File ? "Ok" : "OK";
	return "";
    }

    return 0;
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


void uiMainWin::getTopLevelWindows( ObjectSet<uiMainWin>& list,
				    bool visibleonly )
{
    uiMainWinBody::getTopLevelWindows( list, visibleonly );
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
	    BufferString qwptrstr( 256, false );
	    qwptrstr.setPtr( qw );
	    signatures.add( qwptrstr.buf() );
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



bool uiMainWin::grab( const char* filenm, int zoom,
		      const char* format, int quality ) const
{
    QScreen* qscreen = qScreen();
    if ( !qscreen )
	return false;

    if ( zoom == 0 )
	return grabScreen( filenm, format, quality, qscreen );
    else if ( zoom == 1 )
	getNonConst(*this).saveImage( filenm, format, quality );
    else
	activeModalWindow()->saveImage( filenm, format, quality );

    return true;
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

    return grabScreen( filenm, format, quality, qscreen );
}


bool uiMainWin::grabScreen( const char* filenm, const char* format, int quality,
			    QScreen* qscreen )
{
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


void setImageProp( QScreen* qscreen, QWidget* qw, int w, int h, int r )
{
    qscreen_ = qscreen;
    qwin_ = qw;
    width_ = w;
    height_ = h;
    res_ = r;
    copytoclipboard_ = true;
    timer_.start( 500, true );
}


void setImageProp( QScreen* qscreen, QWidget* qw, const char* fnm,
		   const char* format, int quality, int w, int h, int r )
{
    qscreen_ = qscreen;
    qwin_ = qw;
    fname_ = fnm;
    format_ = format;
    quality_ = quality;
    width_ = w;
    height_ = h;
    res_ = r;
    copytoclipboard_ = false;
    timer_.start( 500, true );
}


protected:
void shootImageCB( CallBacker* )
{
    if ( !qscreen_ || !qwin_ )
	return;

    const QRect fmgeo = qwin_->frameGeometry();
    const QRect geom = qwin_->geometry();
    const int xshift = geom.width() - fmgeo.width();
    const int yshift = geom.height() - fmgeo.height();
    WId wid = qwin_->winId();
    const QPixmap snapshot = qscreen_->grabWindow( wid, xshift, yshift );

    QImage image = snapshot.toImage();
    image = image.scaledToWidth( width_ );
    image = image.scaledToHeight( height_ );
    image.setDotsPerMeterX( (int)(res_/0.0254) );
    image.setDotsPerMeterY( (int)(res_/0.0254) );
    if ( copytoclipboard_ )
	uiClipboard::setImage( image );
    else
	image.save( fname_, format_, quality_ );

    timer_.stop();
}

    int			width_ = 0;
    int			height_ = 0;
    int			xshift_ = 0;
    int			yshift_ = 0;
    int			res_ = -1;
    bool		copytoclipboard_ = false;
    QString		fname_;
    BufferString	format_;
    int			quality_;
    QScreen*		qscreen_;
    QWidget*		qwin_;
    Timer		timer_;
};


void uiMainWin::copyToClipBoard()
{
    QWidget* qwin = qWidget();
    if ( !qwin )
	qwin = body_;

    raise();
    const QRect fmgeo = qwin->frameGeometry();
    mDefineStaticLocalObject( ImageSaver, imagesaver, );
    imagesaver.setImageProp( qScreen(), qwin, fmgeo.width(), fmgeo.height(),
			     uiMain::getMinDPI() );
}


void uiMainWin::saveImage( const char* fnm, const char* format, int quality )
{
    QWidget* qwin = qWidget();
    if ( !qwin )
	qwin = body_;

    raise();
    const QRect fmgeo = qwin->frameGeometry();
    mDefineStaticLocalObject( ImageSaver, imagesaver, );
    imagesaver.setImageProp( qScreen(), qwin, fnm, format, quality,
			 fmgeo.width(), fmgeo.height(), uiMain::getMinDPI() );
}


void uiMainWin::saveImage( const char* fnm, int width, int height, int res )
{
    QWidget* qwin = qWidget();
    if ( !qwin )
	qwin = body_;

    raise();
    mDefineStaticLocalObject( ImageSaver, imagesaver, );
    imagesaver.setImageProp( qScreen(), qwin, fnm, nullptr, -1, width, height,
			     res );
}


void uiMainWin::saveAsPDF( const char* filename, int w, int h, int res )
{
    QWidget* qwin = qWidget();
    if ( qwin )
	saveAsPDF( *qwin, filename, w, h, res );
}


Notifier<uiMainWin>& uiMainWin::iconSetChanged()
{
    static PtrMan<Notifier<uiMainWin> > thenotif_ =
					new Notifier<uiMainWin>( nullptr );
    return *thenotif_.ptr();
}
