/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          09/02/2001
 RCS:           $Id: uitextedit.cc,v 1.7 2002-01-29 11:12:28 bert Exp $
________________________________________________________________________

-*/


#include <uitextedit.h>
#include <uiobjbody.h>
#include <qtextedit.h> 

int uiTextEdit::defaultWidth_	= 600;
int uiTextEdit::defaultHeight_	= 400;

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
: uiObject( parnt, nm, mkbody(parnt, nm, ro) )		{}

uiTextEditBody& uiTextEdit::mkbody(uiParent* parnt, const char* nm, bool ro)
{ 
    body_= new uiTextEditBody( *this, parnt, nm, ro );
    return *body_; 
}


void uiTextEdit::setText( const char* txt)	{ body_->setText(txt); }
void uiTextEdit::append( const char* txt)	{ body_->append(txt); }

const char* uiTextEdit::text() const
{ 
    const_cast<uiTextEdit*>(this)->result = (const char*)body_->text();
    return (const char*)result;
}
