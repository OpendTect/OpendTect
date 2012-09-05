/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          09/02/2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uitextedit.cc,v 1.61 2012-09-05 07:25:52 cvsjaap Exp $";


#include "uitextedit.h"

#include "uiobjbody.h"
#include "uifont.h"
#include "i_qtxtbrowser.h"
#include "i_qtextedit.h"

#include "ascstream.h"
#include "rowcol.h"
#include "strmdata.h"
#include "strmprov.h"
#include "timer.h"
#include "varlenarray.h"

#include <iostream>
#include <QScrollBar>
#include <QTextDocument> 
#include <QTextEdit> 
#include <QToolTip> 


#define mMaxLineLength 32768


uiTextEditBase::uiTextEditBase( uiParent* p, const char* nm, uiObjectBody& bdy )
    : uiObject(p,nm,bdy)
    , defaultwidth_(600)
    , defaultheight_(450)
{
    setFont( FontList().get(FontData::key(FontData::Fixed)) );
    setPrefWidth( defaultwidth_ );
    setPrefHeight( defaultheight_ );
}


const char* uiTextEditBase::text() const
{ 
    result_ = mQStringToConstChar( qte().toPlainText() );
    return result_.buf();
}


bool uiTextEditBase::isModified() const
{ return qte().document()->isModified(); }


void uiTextEditBase::allowTextSelection( bool yn )
{
    const mQtclass(Qt)::TextInteractionFlags selflag =
					mQtclass(Qt)::TextSelectableByMouse;
    const mQtclass(Qt)::TextInteractionFlags mask = ~selflag;
    mQtclass(Qt)::TextInteractionFlags flags =
					qte().textInteractionFlags() & mask;
    
    if ( yn )
        flags |= selflag;
    
    qte().setTextInteractionFlags( flags );
}

void uiTextEditBase::hideFrame()
{ qte().setFrameShape( mQtclass(QFrame)::NoFrame ); }

void uiTextEditBase::hideScrollBar( bool vertical )
{
    if ( vertical )
	qte().setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    else
	qte().setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
}


void uiTextEditBase::readFromFile( const char* src, int wraplen )
{
    StreamData sd = StreamProvider( src ).makeIStream();
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
	if ( !sd.istrm->getline(buf,mMaxLineLength) )
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
    StreamData sd = StreamProvider( src ).makeOStream();
    if ( !sd.usable() )
	{ sd.close(); return false; }

    if ( linelen < 1 && newlns )
	*sd.ostrm << text();
    else
    {
	mAllocVarLenArr( char, fullline, linelen+1 );
	BufferString inptxt( text() );
	char* ptr = inptxt.buf();
	while ( ptr && *ptr )
	{
	    char* startptr = ptr;
	    ptr = strchr( ptr, '\n' );
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
		*sd.ostrm << startptr;
	    else
	    {
		memset( fullline, ' ', linelen ); fullline[linelen] = '\0';
		strncpy( fullline, startptr, lnlen );
		*sd.ostrm << fullline;
	    }

	    if ( newlns ) *sd.ostrm << '\n';
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


class uiTextEditBody : public uiObjBodyImpl<uiTextEdit,mQtclass(QTextEdit)>
{
public:

                        uiTextEditBody(uiTextEdit&,uiParent*,
				       const char* nm,bool ro);
			~uiTextEditBody()	{ delete &messenger_; }

    void		append(const char*);

protected:
    i_TextEditMessenger& messenger_;
};


uiTextEditBody::uiTextEditBody( uiTextEdit& hndl, uiParent* p, 
				const char* nm, bool ro )
    : uiObjBodyImpl<uiTextEdit,mQtclass(QTextEdit)>( hndl, p, nm )
    , messenger_(*new i_TextEditMessenger(this,&hndl))
{
    setReadOnly( ro );
    setStretch( 2, 2 );
}


void uiTextEditBody::append( const char* txt)
{ 
    mQtclass(QTextEdit)::append( txt );
    repaint();
    moveCursor( mQtclass(QTextCursor)::End );
}

//-------------------------------------------------------

uiTextEdit::uiTextEdit( uiParent* parnt, const char* nm, bool ro )
    : uiTextEditBase( parnt, nm, mkbody(parnt,nm,ro) )		
    , textChanged(this)
{
    setPrefWidth( defaultWidth() );
    setPrefHeight( defaultHeight() );
}


uiTextEditBody& uiTextEdit::mkbody(uiParent* parnt, const char* nm, bool ro)
{ 
    body_= new uiTextEditBody( *this, parnt, nm, ro );
    return *body_; 
}


void uiTextEdit::setText( const char* txt, bool trigger_notif )
{
    NotifyStopper ns( textChanged );
    if ( trigger_notif ) ns.restore();
    qte().setText( txt );
}


void uiTextEdit::append( const char* txt )	{ body_->append(txt); }

mQtclass(QTextEdit&) uiTextEdit::qte()			{ return *body_; }



//-------------------------------------------------------


class uiTextBrowserBody : public uiObjBodyImpl<uiTextBrowser,
    					       mQtclass(QTextBrowser)>
{
public:

                        uiTextBrowserBody(uiTextBrowser&,uiParent*,const char*,
					  bool plaintxt );

    virtual		~uiTextBrowserBody()	{ delete &messenger_; }

    void		recordScrollPos();
    void		restoreScrollPos();
protected:

    i_BrowserMessenger& messenger_;
    RowCol		scrollpos_;
};


uiTextBrowserBody::uiTextBrowserBody( uiTextBrowser& hndl, uiParent* p, 
				      const char* nm, bool plaintxt )
    : uiObjBodyImpl<uiTextBrowser,mQtclass(QTextBrowser)>( hndl, p, nm )
    , messenger_( *new i_BrowserMessenger(this, &hndl))
    , scrollpos_(mUdf(int),mUdf(int))
{
    setStretch( 2, 2 );
}


void uiTextBrowserBody::recordScrollPos()
{
    scrollpos_.row = horizontalScrollBar() ? horizontalScrollBar()->value()
					   : mUdf(int);
    scrollpos_.col = verticalScrollBar() ? verticalScrollBar()->value()
					   : mUdf(int);
}


void uiTextBrowserBody::restoreScrollPos()
{
    if ( horizontalScrollBar() && !mIsUdf(scrollpos_.row) )
	horizontalScrollBar()->setValue( scrollpos_.row );
    if ( verticalScrollBar() && !mIsUdf(scrollpos_.col) )
	verticalScrollBar()->setValue( scrollpos_.col );
}


//-------------------------------------------------------

uiTextBrowser::uiTextBrowser( uiParent* parnt, const char* nm, int mxlns,
			      bool forceplaintxt, bool lvmode )
    : uiTextEditBase( parnt, nm, mkbody(parnt,nm,forceplaintxt) )	
    , goneForwardOrBack(this)
    , linkHighlighted(this)
    , linkClicked(this)
    , cangoforw_(false)
    , cangobackw_(false)
    , forceplaintxt_(forceplaintxt)
    , maxlines_(mxlns)
    , logviewmode_(lvmode)
    , lastlinestartpos_(-1)
{
    if ( !mIsUdf(mxlns) )
	qte().document()->setMaximumBlockCount( mxlns+2 );

    timer_ = new Timer();
    timer_->tick.notify( mCB(this,uiTextBrowser,readTailCB) );
}


uiTextBrowser::~uiTextBrowser()
{
    timer_->tick.remove( mCB(this,uiTextBrowser,readTailCB) );
    delete timer_;
}


uiTextBrowserBody& uiTextBrowser::mkbody( uiParent* parnt, const char* nm,
					  bool forceplaintxt )
{ 
    body_ = new uiTextBrowserBody( *this, parnt, nm, forceplaintxt );
    return *body_; 
}


mQtclass(QTextEdit&) uiTextBrowser::qte()	{ return *body_; }


void uiTextBrowser::showToolTip( const char* txt )
{
    mQtclass(QToolTip)::showText( mQtclass(QCursor)::pos(),
	    			  mQtclass(QString)(txt) );
}


void uiTextBrowser::readTailCB( CallBacker* )
{
    StreamData sd = StreamProvider( textsrc_ ).makeIStream();
    if ( !sd.usable() )
	return;

    char buf[mMaxLineLength];
    const int maxchartocmp = mMIN( mMaxLineLength, 80 );

    if ( lastlinestartpos_ >= 0 )
    {
	sd.istrm->seekg( lastlinestartpos_ );
	sd.istrm->getline( buf, mMaxLineLength );
	if ( !sd.istrm->good() || strncmp(buf, lastline_.buf(), maxchartocmp) )
	{
	    sd.close();
	    lastlinestartpos_ = -1;
	    qte().setText( "" );
	    sd = StreamProvider( textsrc_ ).makeIStream();
	    if ( !sd.usable() )
		return;
	}
    }

    while ( sd.istrm->peek()!=EOF )
    {
	lastlinestartpos_ = sd.istrm->tellg();
	sd.istrm->getline( buf, mMaxLineLength );
	qte().append( buf );
    }

    buf[maxchartocmp-1] = '\0';
    lastline_= buf;
    sd.close();
}


void uiTextBrowser::setText( const char* txt )
{ qte().setText( txt ); }

void uiTextBrowser::setHtmlText( const char* txt )
{
    body_->setHtml( txt );
}


void uiTextBrowser::getHtmlText( BufferString& res ) const
{
    const mQtclass(QString) str = body_->toHtml();
    res = str.toAscii().data();
}



void  uiTextBrowser::setLinkBehavior( uiTextBrowser::LinkBehavior lb )
{
    body_->setOpenExternalLinks( lb==FollowAll );
    body_->setOpenLinks( lb!=None );
}


const char* uiTextBrowser::source() const
{ 
    if ( forceplaintxt_ )
	return textsrc_;


    result_ = body_->source().path().toAscii().data();
    return result_.buf();
}


void uiTextBrowser::setSource( const char* src )
{
    if ( forceplaintxt_ )
    {
	textsrc_ = src;

	if ( logviewmode_ )
	{
	    qte().setText( "" );
	    lastlinestartpos_ = -1;
	    readTailCB( 0 );
	    timer_->start( 500, false ); 
	}
	else
	    readFromFile( src );
    }
    else
        body_->setSource( mQtclass(QUrl)(src) );
}


void uiTextBrowser::setMaxLines( int ml )
{ maxlines_ = ml; }

void uiTextBrowser::backward()
{ body_->backward();}

void uiTextBrowser::forward()
{ body_->forward(); }

void uiTextBrowser::home()
{ body_->home(); }

void uiTextBrowser::scrollToBottom()
{ body_->moveCursor( mQtclass(QTextCursor)::End ); }

void uiTextBrowser::recordScrollPos()
{ body_->recordScrollPos(); }

void uiTextBrowser::restoreScrollPos()
{ body_->restoreScrollPos(); }


void uiTextBrowser::reload()
{
    if ( forceplaintxt_ )
	setSource( textsrc_ );

    body_->reload();
}
