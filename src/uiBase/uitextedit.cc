/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          09/02/2001
 RCS:           $Id: uitextedit.cc,v 1.18 2003-02-27 11:39:12 arend Exp $
________________________________________________________________________

-*/


#include <uitextedit.h>
#include <uiobjbody.h>
#include <qtextedit.h> 
#include <i_qtxtbrowser.h>

#include <qstringlist.h>

#define WORK_AROUND
#ifdef WORK_AROUND
#include <strmprov.h>
#include <strmdata.h>
#include <ascstream.h>
#endif

int uiTextEditBase::defaultWidth_	= 600;
int uiTextEditBase::defaultHeight_	= 400;

uiTextEditBase::uiTextEditBase( uiParent* p, const char* nm, uiObjectBody& bdy )
    : uiObject(p,nm,bdy)
{}


void uiTextEditBase::setText( const char* txt)	{ qte().setText(txt); }

const char* uiTextEditBase::text() const
{ 
    result = (const char*)qte().text();
    return (const char*)result;
}



class uiTextEditBody : public uiObjBodyImpl<uiTextEdit,QTextEdit>
{
public:

                        uiTextEditBody( uiTextEdit& handle, uiParent* parnt, 
					const char* nm, bool ro );

    void		append( const char* ); 

};


uiTextEditBody::uiTextEditBody( uiTextEdit& handle, uiParent* p, 
				const char* nm, bool ro )
    : uiObjBodyImpl<uiTextEdit,QTextEdit>( handle, p, nm )
{
    setTextFormat(Qt::PlainText); 
    setReadOnly( ro );
    setStretch( 2, 2 );
    setPrefWidth( handle.defaultWidth() );
    setPrefHeight( handle.defaultHeight() );
}



void uiTextEditBody::append( const char* txt)
{ 
    QTextEdit::append( txt );
    repaint();
    setCursorPosition( lines(), 0 );
} 

//-------------------------------------------------------

uiTextEdit::uiTextEdit(uiParent* parnt, const char* nm, bool ro)
: uiTextEditBase( parnt, nm, mkbody(parnt, nm, ro) )		{}

uiTextEditBody& uiTextEdit::mkbody(uiParent* parnt, const char* nm, bool ro)
{ 
//    QStringList _path(".");
    body_= new uiTextEditBody( *this, parnt, nm, ro );
//    body_->mimeSourceFactory()->setFilePath( _path );
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

    mimeSourceFactory()->setExtensionType( "par", "text/plain" );
    mimeSourceFactory()->setExtensionType( "log", "text/plain" );
    mimeSourceFactory()->setExtensionType( "sim", "text/plain" );
    mimeSourceFactory()->setExtensionType( "fw", "text/plain" );
    mimeSourceFactory()->setExtensionType( "nn", "text/plain" );
    mimeSourceFactory()->setExtensionType( "dict", "text/plain" );

    mimeSourceFactory()->addFilePath ( "." );

    setStretch( 2, 2 );
    setPrefWidth( handle.defaultWidth() );
    setPrefHeight( handle.defaultHeight() );
}




//-------------------------------------------------------

uiTextBrowser::uiTextBrowser(uiParent* parnt, const char* nm, bool forcePTxt )
    : uiTextEditBase( parnt, nm, mkbody(parnt, nm, forcePTxt) )	
    , goneforwardorback(this)
    , linkhighlighted(this)
    , linkclicked(this)
    , cangoforw_( false )
    , cangobackw_( false )
    , forceplaintxt_( forcePTxt )

{}


uiTextBrowserBody& uiTextBrowser::mkbody( uiParent* parnt, const char* nm,
					  bool forcePlainText )
{ 
    body_= new uiTextBrowserBody( *this, parnt, nm, forcePlainText );
    return *body_; 
}

QTextEdit& uiTextBrowser::qte()				{ return *body_; }

#ifdef WORK_AROUND
static BufferString thesrc;
#endif

const char* uiTextBrowser::source() const
{ 
#ifdef WORK_AROUND
    if ( forceplaintxt_ )
	return thesrc;
#endif
    result = (const char*)body_->source();
    return (const char*)result;
}


void uiTextBrowser::setSource( const char* src )
{
#ifdef WORK_AROUND
    if ( forceplaintxt_ )
    {
	StreamData sd = StreamProvider( src ).makeIStream();
	if ( !sd.istrm || sd.istrm->fail() )
	    { sd.close(); return; }

	thesrc = src;
	BufferString contents;

	char buf[1024]; int maxlines=65536;
	while ( sd.istrm->getline(buf,1024) && maxlines-- >= 0 )
	    { contents += buf; contents += "\n"; }

	sd.close();

	body_->setText( QString((const char*)contents) );
    }
    else
#endif
    body_->setSource(src);
}


void uiTextBrowser::backward()
{ body_->backward();}


void uiTextBrowser::forward()
{ body_->forward(); }


void uiTextBrowser::home()
{ body_->home(); }


void uiTextBrowser::reload()
{
#ifdef WORK_AROUND
    if ( forceplaintxt_ )
	setSource(thesrc);
#endif
    body_->reload();
}


int uiTextBrowser::nrLines()
{ return body_->lines(); }


void uiTextBrowser::scrollToBottom()
{ body_->scrollToBottom(); }
