/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          09/02/2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uitextedit.cc,v 1.43 2009-07-22 16:01:38 cvsbert Exp $";


#include "uitextedit.h"

#include "uiobjbody.h"
#include "uifont.h"
#include "i_qtxtbrowser.h"

#include "ascstream.h"
#include "strmdata.h"
#include "strmprov.h"

#include <iostream>
#include <QTextDocument> 
#include <QTextEdit> 



uiTextEditBase::uiTextEditBase( uiParent* p, const char* nm, uiObjectBody& bdy )
    : uiObject(p,nm,bdy)
    , defaultwidth_(600)
    , defaultheight_(450)
{
    setFont( FontList().get(FontData::key(FontData::Fixed)) );
    setPrefWidth( defaultwidth_ );
    setPrefHeight( defaultheight_ );
}


void uiTextEditBase::setText( const char* txt )
{ qte().setText( txt ); }


const char* uiTextEditBase::text() const
{ 
    result_ = mQStringToConstChar( qte().toPlainText() );
    return result_.buf();
}


bool uiTextEditBase::isModified() const
{ return qte().document()->isModified(); }


void uiTextEditBase::readFromFile( const char* src, int wraplen )
{
    StreamData sd = StreamProvider( src ).makeIStream();
    if ( !sd.usable() )
	{ sd.close(); return; }

    BufferString contents;
    BufferString newcontents;

#define mMaxLineLength 32768
    char buf[mMaxLineLength];
    int lines_left = maxLines();
    if ( lines_left < 0 ) lines_left = mUdf(int);
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

    sd.close();
    setText( contents );
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
	BufferString inptxt( text() );
	char* ptr = inptxt.buf();
	while ( *ptr )
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
	    *sd.ostrm << startptr;
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
    setReadOnly( ro );
    setStretch( 2, 2 );
}


void uiTextEditBody::append( const char* txt)
{ 
    QTextEdit::append( txt );
    repaint();
    moveCursor( QTextCursor::End );
}

//-------------------------------------------------------

uiTextEdit::uiTextEdit( uiParent* parnt, const char* nm, bool ro )
    : uiTextEditBase( parnt, nm, mkbody(parnt,nm,ro) )		
{
    setPrefWidth( defaultWidth() );
    setPrefHeight( defaultHeight() );
}


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
    setStretch( 2, 2 );
}




//-------------------------------------------------------

uiTextBrowser::uiTextBrowser( uiParent* parnt, const char* nm, int mxlns,
			      bool forceplaintxt )
    : uiTextEditBase( parnt, nm, mkbody(parnt,nm,forceplaintxt) )	
    , goneForwardOrBack(this)
    , linkHighlighted(this)
    , linkClicked(this)
    , cangoforw_(false)
    , cangobackw_(false)
    , forceplaintxt_(forceplaintxt)
    , maxlines_(mxlns)
{
}


uiTextBrowserBody& uiTextBrowser::mkbody( uiParent* parnt, const char* nm,
					  bool forceplaintxt )
{ 
    body_ = new uiTextBrowserBody( *this, parnt, nm, forceplaintxt );
    return *body_; 
}


QTextEdit& uiTextBrowser::qte()	{ return *body_; }


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
	readFromFile( src );
    }
    else
        body_->setSource( QUrl(src) );
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
{ body_->moveCursor( QTextCursor::End ); }


void uiTextBrowser::reload()
{
    if ( forceplaintxt_ )
	setSource( textsrc_ );

    body_->reload();
}
