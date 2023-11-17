/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitextedit.h"
#include "i_qtextedit.h"

#include "uifont.h"
#include "uilabel.h"
#include "uimain.h"
#include "uiobjbodyimpl.h"

#include "ascstream.h"
#include "rowcol.h"
#include "strmprov.h"
#include "timer.h"
#include "varlenarray.h"

#include "q_uiimpl.h"

#include <iostream>
#include <QScrollBar>
#include <QTextDocument>
#include <QTextEdit>
#include <QToolTip>
#include <QWheelEvent>
#include <stdio.h> // for EOF

mUseQtnamespace

#define mMaxLineLength 32768


uiTextEditBase::uiTextEditBase( uiParent* p, const char* nm, uiObjectBody& bdy )
    : uiObject(p,nm,bdy)
    , defaultwidth_(600)
    , defaultheight_(450)
    , textChanged(this)
    , sliderPressed(this)
    , sliderReleased(this)
    , copyAvailable(this)
{
    setFont( FontList().get(FontData::Fixed) );
    setPrefWidth( defaultwidth_ );
    setPrefHeight( defaultheight_ );
}


uiTextEditBase::~uiTextEditBase()
{}


void uiTextEditBase::setEmpty()
{
    qte().clear();
}


const char* uiTextEditBase::text() const
{
    result_ = qte().toPlainText();
    return result_.buf();
}


bool uiTextEditBase::isModified() const
{ return qte().document()->isModified(); }


void uiTextEditBase::allowTextSelection( bool yn )
{
    const Qt::TextInteractionFlags selflag = Qt::TextSelectableByMouse;
    const Qt::TextInteractionFlags mask = ~selflag;
    Qt::TextInteractionFlags flags = qte().textInteractionFlags() & mask;

    if ( yn )
	flags |= selflag;

    qte().setTextInteractionFlags( flags );
}

void uiTextEditBase::hideFrame()
{
    qte().setFrameShape( QFrame::NoFrame );
}


void uiTextEditBase::setLineWrapColumn( int nrcol )
{
    qte().setLineWrapMode( QTextEdit::FixedColumnWidth );
    qte().setLineWrapColumnOrWidth( nrcol );
}


bool uiTextEditBase::verticalSliderIsDown() const
{
    QScrollBar* verticalscrollbar = qte().verticalScrollBar();

    return verticalscrollbar && verticalscrollbar->isSliderDown();
}


void uiTextEditBase::scrollToBottom()
{
    QScrollBar* verticalscrollbar = qte().verticalScrollBar();
    if ( !verticalscrollbar )
	return;

    verticalscrollbar->setSliderDown( true );
}


void uiTextEditBase::hideScrollBar( bool vertical )
{
    if ( vertical )
	qte().setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    else
	qte().setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
}


void uiTextEditBase::setPlaceholderText( const uiString& txt )
{
    qte().setPlaceholderText( toQString(txt) );
}


void uiTextEditBase::readFromFile( const char* src, int wraplen )
{
    StreamData sd = StreamProvider::createIStream( src );
    if ( !sd.usable() )
	{ sd.close(); return; }

    BufferString contents;
    BufferString newcontents;

    char buf[mMaxLineLength];
    int lines_left = maxLines();
    if ( lines_left < 0 )
	lines_left = mUdf(int);
    int nrnewlines = 0;
    while ( true )
    {
	if ( !sd.iStrm()->getline(buf,mMaxLineLength) )
	    break;

	lines_left--;
	if ( lines_left < 0 )
	{
	    newcontents += "\n-------- More lines follow -----";
	    break;
	}
	else
	{
	    const int buflen = strlen( buf );
	    if ( wraplen < 1 || buflen < wraplen )
		newcontents += buf;
	    else
	    {
		char* ptr = buf;
		int lenleft = buflen;
		while ( lenleft > 0 )
		{
		    if ( lenleft <= wraplen )
			newcontents += ptr;
		    else
		    {
			const char kp = ptr[wraplen];
			ptr[wraplen] = '\0';
			newcontents += ptr;
			newcontents += "\n";
			nrnewlines++;
			ptr += wraplen;
			*ptr = kp;
		    }
		    lenleft -= wraplen;
		}
	    }
	    newcontents += "\n";
	}
	nrnewlines++;
	if ( nrnewlines == 100 )
	{
	    contents += newcontents;
	    newcontents = "";
	    nrnewlines = 0;
	}
    }

    if ( !newcontents.isEmpty() )
	contents += newcontents;

    qte().setText( contents.buf() );
    sd.close();
}


bool uiTextEditBase::saveToFile( const char* src, int linelen, bool newlns )
{
    StreamData sd = StreamProvider::createOStream( src );
    if ( !sd.usable() )
	{ sd.close(); return false; }

    if ( linelen < 1 && newlns )
	*sd.oStrm() << text();
    else
    {
	mAllocVarLenArr( char, fullline, linelen+1 );
	BufferString inptxt( text() );
	char* ptr = inptxt.getCStr();
	while ( ptr && *ptr )
	{
	    char* startptr = ptr;
	    ptr = firstOcc( ptr, '\n' );
	    if ( ptr )
	    {
		*ptr++ = '\0';
		if ( linelen > 0 )
		{
		    const int lnlen = strlen( startptr );
		    if ( lnlen > linelen )
			startptr[linelen] = '\0';
		}
	    }

	    const int lnlen = strlen( startptr );
	    if ( linelen < 1 || lnlen==linelen )
		*sd.oStrm() << startptr;
	    else
	    {
		OD::memSet( fullline, ' ', linelen );
		fullline[linelen] = '\0';
#ifdef __win__
		strncpy_s( fullline, linelen+1, startptr, lnlen );
#else
		strncpy( fullline, startptr, lnlen );
#endif
		*sd.oStrm() << fullline;
	    }

	    if ( newlns ) *sd.oStrm() << '\n';
	}
    }

    sd.close();

    qte().document()->setModified( false );
    return true;
}


int uiTextEditBase::nrLines() const
{
    return qte().document()->blockCount();
}


class uiTextEditBody : public uiObjBodyImpl<uiTextEdit,QTextEdit>
{
public:

			uiTextEditBody(uiTextEdit&,uiParent*,
				       const char* nm,bool ro);
			~uiTextEditBody()	{ delete &messenger_; }

    void		append(const char*);
    void		ignoreWheelEvents(bool);

protected:

    i_TextEditMessenger& messenger_;
    bool		ignorewheelevents_	= false;

    void		wheelEvent(QWheelEvent*) override;

};


uiTextEditBody::uiTextEditBody( uiTextEdit& hndl, uiParent* p,
				const char* nm, bool ro )
    : uiObjBodyImpl<uiTextEdit,QTextEdit>( hndl, p, nm )
    , messenger_(*new i_TextEditMessenger(this,&hndl))
{
    setStretch( 2, 2 );
    setReadOnly( ro );
}


void uiTextEditBody::append( const char* txt )
{
    const bool sliderwasdown = handle_.verticalSliderIsDown();
    QTextEdit::append( txt );
    repaint();
    if ( sliderwasdown )
	handle_.scrollToBottom();
}


void uiTextEditBody::ignoreWheelEvents( bool yn )
{
    ignorewheelevents_ = yn;
}


void uiTextEditBody::wheelEvent( QWheelEvent* ev )
{
    if ( ignorewheelevents_ && ev )
	ev->ignore();
    else
	QTextEdit::wheelEvent( ev );
}

//-------------------------------------------------------

uiTextEdit::uiTextEdit( uiParent* parnt, const char* nm, bool ro )
    : uiTextEditBase( parnt, nm, mkbody(parnt,nm,ro) )
{
    setPrefWidth( defaultWidth() );
    setPrefHeight( defaultHeight() );
    if ( ro )
	setBackgroundColor( roBackgroundColor() );

    const int defzoom = mNINT32( uiMain::getDefZoomLevel() );
    if ( defzoom > 1 )
	body_->zoomIn( defzoom );
}


uiTextEdit::~uiTextEdit()
{}


uiTextEditBody& uiTextEdit::mkbody(uiParent* parnt, const char* nm, bool ro)
{
    body_= new uiTextEditBody( *this, parnt, nm, ro );
    return *body_;
}



uiLabel* uiTextEdit::setIcon( const char* iconnm, const uiString& tooltip )
{
    auto* lbl = new uiLabel( parent(), uiString::emptyString() );
    lbl->setIcon( iconnm );
    lbl->setToolTip( tooltip );
    lbl->attach( leftOf, this );
    return lbl;
}


void uiTextEdit::setText( const char* txt, bool trigger_notif )
{
    if ( trigger_notif )
    {
	qte().setPlainText( txt );
	qte().setFontUnderline( false );
    }
    else
    {
	NotifyStopper ns( textChanged, this );
	qte().setPlainText( txt );
	qte().setFontUnderline( false );
    }
}


void uiTextEdit::setText( const OD::String& txt )
{
    setText( txt.buf() );
}


void uiTextEdit::setText( const uiString& txt )
{
    NotifyStopper ns( textChanged );
    qte().setText( toQString(txt) );
    body_->setReadOnly( true );
    setBackgroundColor( roBackgroundColor() );
}


void uiTextEdit::append( const char* txt )
{
    body_->append( txt );
}


QTextEdit& uiTextEdit::qte()
{
    return *body_;
}


void uiTextEdit::ignoreWheelEvents( bool yn )
{
    body_->ignoreWheelEvents( yn );
}


void uiTextEdit::setWordWrapMode( WrapMode mode )
{
    body_->setWordWrapMode( QTextOption::WrapMode(mode) );
}


//-------------------------------------------------------


class uiTextBrowserBody : public uiObjBodyImpl<uiTextBrowser,QTextBrowser>
{
public:

			uiTextBrowserBody(uiTextBrowser&,uiParent*,const char*,
					  bool plaintxt );
    virtual		~uiTextBrowserBody();

    void		recordScrollPos();
    void		restoreScrollPos();
    void		ignoreWheelEvents(bool);

protected:

    i_BrowserMessenger& messenger_;
    i_ScrollBarMessenger&	vertscrollbarmessenger_;

private:

    double		horscrollpos_;
    double		vertscrollpos_;
    bool		ignorewheelevents_	= false;

    void		wheelEvent(QWheelEvent*) override;
};


uiTextBrowserBody::uiTextBrowserBody( uiTextBrowser& hndl, uiParent* p,
				      const char* nm, bool plaintxt )
    : uiObjBodyImpl<uiTextBrowser,QTextBrowser>( hndl, p, nm )
    , messenger_( *new i_BrowserMessenger(this, &hndl))
    , vertscrollbarmessenger_(
		    *new i_ScrollBarMessenger(this->verticalScrollBar(),&hndl))
    , horscrollpos_(mUdf(double))
    , vertscrollpos_(mUdf(double))
{
    setStretch( 2, 2 );
}


uiTextBrowserBody::~uiTextBrowserBody()
{
    delete &messenger_;
    delete &vertscrollbarmessenger_;
}


static double getScrollBarRelPos( const QScrollBar* scrollbar )
{
    if ( !scrollbar || scrollbar->maximum() == 0 )
	return mUdf(double);

    const double min = scrollbar->minimum();
    const double max = scrollbar->maximum();
    const double pos = scrollbar->value();
    return ( pos - min ) / ( max - min );
}


void uiTextBrowserBody::recordScrollPos()
{
    horscrollpos_ = getScrollBarRelPos( horizontalScrollBar() );
    vertscrollpos_ = getScrollBarRelPos( verticalScrollBar() );
}


static void restoreScrollBarRelPos( QScrollBar* scrollbar, double pos )
{
    if ( !scrollbar || mIsUdf(pos) )
	return;

    const int minline = scrollbar->minimum();
    const int maxline = scrollbar->maximum();

    int line = mNINT32( pos * ( maxline - minline ) );
    line = mMAX(line,minline);
    line = mMIN(line,maxline);
    scrollbar->setValue( line );
}


void uiTextBrowserBody::restoreScrollPos()
{
    restoreScrollBarRelPos( horizontalScrollBar(), horscrollpos_ );
    restoreScrollBarRelPos( verticalScrollBar(), vertscrollpos_ );
}


void uiTextBrowserBody::ignoreWheelEvents( bool yn )
{
    ignorewheelevents_ = yn;
}


void uiTextBrowserBody::wheelEvent( QWheelEvent* ev )
{
    if ( ignorewheelevents_ && ev )
	ev->ignore();
    else
	QTextEdit::wheelEvent( ev );
}


//-------------------------------------------------------

uiTextBrowser::uiTextBrowser( uiParent* parnt, const char* nm, int mxlns,
			      bool forceplaintxt, bool lvmode )
    : uiTextEditBase( parnt, nm, mkbody(parnt,nm,true) )
    , goneForwardOrBack(this)
    , linkHighlighted(this)
    , linkClicked(this)
    , fileReOpened(this)
    , cangoforw_(false)
    , cangobackw_(false)
    , forceplaintxt_(forceplaintxt)
    , maxlines_(mxlns)
    , logviewmode_(lvmode)
    , lastlinestartpos_(-1)
    , timer_(0)
{
    if ( !mIsUdf(mxlns) )
	qte().document()->setMaximumBlockCount( mxlns+2 );

    if ( lvmode )
    {
	timer_ = new Timer( "Read log file tail" );
	mAttachCB( timer_->tick, uiTextBrowser::readTailCB );
	mAttachCB( sliderPressed, uiTextBrowser::sliderPressedCB );
	mAttachCB( sliderReleased, uiTextBrowser::sliderReleasedCB );
	mAttachCB( copyAvailable, uiTextBrowser::copyAvailableCB );
    }

    setBackgroundColor( roBackgroundColor() );
    const int defzoom = mNINT32( uiMain::getDefZoomLevel() );
    if ( defzoom > 1 )
	body_->zoomIn( defzoom );
}


uiTextBrowser::~uiTextBrowser()
{
    detachAllNotifiers();
    delete timer_;
}


uiTextBrowserBody& uiTextBrowser::mkbody( uiParent* parnt, const char* nm,
					  bool forceplaintxt )
{
    body_ = new uiTextBrowserBody( *this, parnt, nm, forceplaintxt );
    return *body_;
}


QTextEdit& uiTextBrowser::qte()	{ return *body_; }


void uiTextBrowser::showToolTip( const char* txt )
{
    QToolTip::showText( QCursor::pos(), QString(txt) );
}


void uiTextBrowser::sliderPressedCB( CallBacker* )
{
    enableTailRead( false );
}


void uiTextBrowser::sliderReleasedCB( CallBacker* )
{
    enableTailRead( true );
}


void uiTextBrowser::copyAvailableCB( CallBacker* cb )
{
    mCBCapsuleUnpack(bool,yn,cb);
    enableTailRead( !yn );
}


void uiTextBrowser::enableTailRead( bool yn )
{
    if ( !logviewmode_ )
	return;

    if ( yn )
	timer_->start( 500, false );
    else
	timer_->stop();
}


void uiTextBrowser::readTailCB( CallBacker* )
{
    StreamData sd = StreamProvider::createIStream( textsrc_ );
    if ( !sd.usable() )
	return;

    char buf[mMaxLineLength];
    const int maxchartocmp = mMIN( mMaxLineLength, 80 );

    recordScrollPos();
    if ( lastlinestartpos_ >= 0 )
    {
	sd.iStrm()->seekg( 0, sd.iStrm()->end );
	const od_int64 filelength = sd.iStrm()->tellg();
	if ( filelength < lastlinestartpos_ )
	{
	    fileReOpened.trigger();
	    lastlinestartpos_ = -1;
	}
	else if ( filelength-1 == lastlinestartpos_ )
	{
	    sd.close();
	    return;
	}

	sd.iStrm()->seekg( lastlinestartpos_ );
	sd.iStrm()->getline( buf, mMaxLineLength );
	if ( !sd.iStrm()->good() || strncmp(buf, lastline_.buf(), maxchartocmp))
	{
	    sd.close();
	    lastlinestartpos_ = -1;
	    setText( 0 );
	    sd = StreamProvider::createIStream( textsrc_ );
	    if ( !sd.usable() )
		return;
	}
    }

    NotifyStopper ns( textChanged );
    while ( sd.iStrm()->peek()!=EOF )
    {
	lastlinestartpos_ = sd.iStrm()->tellg();
	sd.iStrm()->getline( buf, mMaxLineLength );
	qte().append( buf );
    }

    restoreScrollPos();

    buf[maxchartocmp-1] = '\0';
    lastline_= buf;
    sd.close();
}


void uiTextBrowser::setText( const char* txt )
{
    NotifyStopper ns( textChanged );
    if ( !txt )
	qte().clear();

    qte().setText( txt );
}

void uiTextBrowser::setHtmlText( const char* txt )
{ body_->setHtml( txt ); }

void uiTextBrowser::getHtmlText( BufferString& res ) const
{ res = body_->toHtml(); }


void  uiTextBrowser::setLinkBehavior( uiTextBrowser::LinkBehavior lb )
{
    body_->setOpenExternalLinks( lb==FollowAll );
    body_->setOpenLinks( lb!=None );
}


const char* uiTextBrowser::source() const
{
    if ( forceplaintxt_ )
	return textsrc_;


    result_ = body_->source().path();
    return result_.buf();
}


void uiTextBrowser::setSource( const char* src )
{
    if ( forceplaintxt_ )
    {
	textsrc_ = src;

	if ( logviewmode_ )
	{
	    setText( 0 );
	    lastlinestartpos_ = -1;
	    readTailCB( 0 );
	    if ( !timer_->isActive() )
		timer_->start( 500, false );
	}
	else
	    readFromFile( src );
    }
    else
	body_->setSource( QUrl(src) );
}


void uiTextBrowser::setMaxLines( int ml )
{
    maxlines_ = ml;
}


void uiTextBrowser::backward()
{
    body_->backward();
}


void uiTextBrowser::forward()
{
    body_->forward();
}


void uiTextBrowser::home()
{
    body_->home();
}


void uiTextBrowser::recordScrollPos()
{
    body_->recordScrollPos();
}


void uiTextBrowser::restoreScrollPos()
{
    body_->restoreScrollPos();
}


void uiTextBrowser::ignoreWheelEvents( bool yn )
{
    body_->ignoreWheelEvents( yn );
}


void uiTextBrowser::reload()
{
    if ( forceplaintxt_ )
	setSource( textsrc_ );

    body_->reload();
}
