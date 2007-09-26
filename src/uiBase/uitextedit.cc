/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          09/02/2001
 RCS:           $Id: uitextedit.cc,v 1.34 2007-09-26 11:16:04 cvsbert Exp $
________________________________________________________________________

-*/


#include "uitextedit.h"
#include "uiobjbody.h"
#include "i_qtxtbrowser.h"

#include "strmprov.h"
#include "strmdata.h"
#include "ascstream.h"
#include <iostream>

#ifdef USEQT3
# include <qtextedit.h> 
#else
# include <QTextEdit> 
#endif


int uiTextEditBase::defaultWidth_ = 600;
int uiTextEditBase::defaultHeight_ = 450;
// mMaxLineLength defined in ascstream.h


uiTextEditBase::uiTextEditBase( uiParent* p, const char* nm, uiObjectBody& bdy )
    : uiObject(p,nm,bdy)
{
}


void uiTextEditBase::setText( const char* txt )
{
    qte().setText(txt);
}


const char* uiTextEditBase::text() const
{ 
    result = (const char*)qte().text();
    return (const char*)result;
}


bool uiTextEditBase::isModified() const
{ 
    return qte().isModified();
}


void uiTextEditBase::readFromFile( const char* src )
{
    StreamData sd = StreamProvider( src ).makeIStream();
    if ( !sd.usable() )
	{ sd.close(); return; }

    BufferString contents;
    BufferString newcontents;

#define mMaxLineLength 8192
    char buf[mMaxLineLength];
    int lines_left = maxLines();
    if ( lines_left < 0 ) lines_left = mUdf(int);
    int nrnewlines = 0;
    while ( 1 )
    {
	if ( !sd.istrm->getline(buf,mMaxLineLength) )
	    break;

	lines_left--;
	if ( lines_left < 0 )
	{
	    newcontents += "\n-------- More lines follow -----";
	    break;
	}
	{
	    newcontents += buf;
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

    sd.close();
    setText( contents );
}


bool uiTextEditBase::saveToFile( const char* src )
{
    StreamData sd = StreamProvider( src ).makeOStream();
    if ( !sd.usable() )
	{ sd.close(); return false; }

    *sd.ostrm << text();
    sd.close();

    qte().setModified( false );
    return true;
}


int uiTextEditBase::nrLines() const
{
#ifdef USEQT3
    return qte().lines();
#else
    QTextBlock block;
    int nrlines = 0;
    for ( QTextBlock block=qte().document()->begin();
				block.isValid(); block=block.next() )
	nrlines++;

    return nrlines;
#endif
}


class uiTextEditBody : public uiObjBodyImpl<uiTextEdit,QTextEdit>
{
public:

                        uiTextEditBody(uiTextEdit&,uiParent*,
				       const char* nm,bool ro);

    void		append(const char*);
};


uiTextEditBody::uiTextEditBody( uiTextEdit& handle, uiParent* p, 
				const char* nm, bool ro )
    : uiObjBodyImpl<uiTextEdit,QTextEdit>( handle, p, nm )
{
    setTextFormat( Qt::PlainText ); 
    setReadOnly( ro );
    setStretch( 2, 2 );
    setPrefWidth( handle.defaultWidth() );
    setPrefHeight( handle.defaultHeight() );
}


void uiTextEditBody::append( const char* txt)
{ 
    QTextEdit::append( txt );
    repaint();
#ifdef USEQT3
    setCursorPosition( lines(), 0 );
#else
    moveCursor( QTextCursor::End );
#endif
}

//-------------------------------------------------------

uiTextEdit::uiTextEdit( uiParent* parnt, const char* nm, bool ro )
    : uiTextEditBase( parnt, nm, mkbody(parnt,nm,ro) )		
{}


uiTextEditBody& uiTextEdit::mkbody(uiParent* parnt, const char* nm, bool ro)
{ 
    body_= new uiTextEditBody( *this, parnt, nm, ro );
    return *body_; 
}


void uiTextEdit::append( const char* txt)	{ body_->append(txt); }
QTextEdit& uiTextEdit::qte()			{ return *body_; }



//-------------------------------------------------------


class uiTextBrowserBody : public uiObjBodyImpl<uiTextBrowser,QTextBrowser>
{
public:

                        uiTextBrowserBody( uiTextBrowser& handle, 
					uiParent* parnt, 
					const char* nm, bool plaintxt );

    virtual		~uiTextBrowserBody()	{ delete &messenger_; }
protected:
    i_BrowserMessenger& messenger_;
};


uiTextBrowserBody::uiTextBrowserBody( uiTextBrowser& handle, uiParent* p, 
				const char* nm, bool plaintxt )
    : uiObjBodyImpl<uiTextBrowser,QTextBrowser>( handle, p, nm )
    , messenger_( *new i_BrowserMessenger(this, &handle))
{
    if( plaintxt ) setTextFormat(Qt::PlainText); 

#ifdef USEQT3
    mimeSourceFactory()->setExtensionType( "par", "text/plain" );
    mimeSourceFactory()->setExtensionType( "log", "text/plain" );
    mimeSourceFactory()->setExtensionType( "sim", "text/plain" );
    mimeSourceFactory()->setExtensionType( "fw", "text/plain" );
    mimeSourceFactory()->setExtensionType( "nn", "text/plain" );
    mimeSourceFactory()->setExtensionType( "dict", "text/plain" );

    mimeSourceFactory()->addFilePath ( "." );
#endif

    setStretch( 2, 2 );
    setPrefWidth( handle.defaultWidth() );
    setPrefHeight( handle.defaultHeight() );
}




//-------------------------------------------------------

uiTextBrowser::uiTextBrowser( uiParent* parnt, const char* nm, int mxlns,
			      bool forcePTxt )
    : uiTextEditBase( parnt, nm, mkbody(parnt,nm,forcePTxt) )	
    , goneforwardorback(this)
    , linkhighlighted(this)
    , linkclicked(this)
    , cangoforw_( false )
    , cangobackw_( false )
    , forceplaintxt_( forcePTxt )
    , maxlines_(mxlns)
{
}


uiTextBrowserBody& uiTextBrowser::mkbody( uiParent* parnt, const char* nm,
					  bool forcePlainText )
{ 
    body_= new uiTextBrowserBody( *this, parnt, nm, forcePlainText );
    return *body_; 
}

QTextEdit& uiTextBrowser::qte()	{ return *body_; }


const char* uiTextBrowser::source() const
{ 
    if ( forceplaintxt_ )
	return textsrc;

#ifdef USEQT3
    result = (const char*)body_->source();
#else
    result = body_->source().path().toAscii().data();
#endif
    return (const char*)result;
}


void uiTextBrowser::setSource( const char* src )
{
    if ( forceplaintxt_ )
    {
	textsrc = src;
	readFromFile( src );
    }
    else
#ifdef USEQT3
	body_->setSource(src);
#else
        body_->setSource( QUrl(src) );
#endif
}


void uiTextBrowser::setMaxLines( int ml )
{
    maxlines_ = ml;
}


void uiTextBrowser::backward()
{ body_->backward();}


void uiTextBrowser::forward()
{ body_->forward(); }


void uiTextBrowser::home()
{ body_->home(); }


void uiTextBrowser::reload()
{
    if ( forceplaintxt_ )
	setSource( textsrc );

    body_->reload();
}


void uiTextBrowser::scrollToBottom()
{
#ifdef USEQT3
    body_->scrollToBottom();
#else
    body_->moveCursor( QTextCursor::End );
#endif
}
