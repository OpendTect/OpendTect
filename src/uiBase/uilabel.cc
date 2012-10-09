/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          7/9/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";


#include "uilabel.h"
#include "uiobjbody.h"
#include "pixmap.h"

#include <qlabel.h> 


class uiLabelBody : public uiObjBodyImpl<uiLabel,QLabel>
{
public:

                        uiLabelBody( uiLabel& hndle, uiParent* parnt,
				     const char* txt )
			    : uiObjBodyImpl<uiLabel,QLabel>(hndle,parnt,txt)
			{}

    virtual int 	nrTxtLines() const		
			{ 
			    int nrl = 1;
			    BufferString str = mQStringToConstChar( text() );
			    char* txt = str.buf();
			    while ( txt && *txt )
				{ if ( *txt == '\n' ) nrl++; txt++; }

			    return nrl; 
			}

};


uiLabel::uiLabel( uiParent* p, const char* txt )
    : uiObject(p,txt,mkbody(p,txt))
{
    init( txt, 0 );
}


uiLabel::uiLabel( uiParent* p, const char* txt, uiGroup* grp )
    : uiObject(p,txt,mkbody(p,txt))
{
    init( txt, grp ? grp->attachObj() : 0 );
}


uiLabel::uiLabel( uiParent* p, const char* txt, uiObject* buddy )
    : uiObject(p,txt,mkbody(p,txt))
{
    init( txt, buddy );
}


void uiLabel::init( const char* txt, uiObject* buddy )
{
    setText( txt );
    setTextSelectable( true );

    if ( buddy ) 
    {
	body_->setBuddy( buddy->body()->qwidget() );
	buddy->attach( rightOf, this );
    }
    setStretch( 0, 0 );
}


uiLabelBody& uiLabel::mkbody( uiParent* p, const char* txt )
{ 
    body_= new uiLabelBody( *this, p, txt );
    return *body_; 
}


void uiLabel::setText( const char* txt )
{ 
    body_->setText( QString(txt) );
    setName( txt );
}


const char* uiLabel::text() const
{
    static BufferString txt;
    txt = mQStringToConstChar( body_->text() );
    return txt.buf();
}


void uiLabel::setTextSelectable( bool yn ) 
{
    body_->setTextInteractionFlags( yn ? Qt::TextSelectableByMouse :
					 Qt::NoTextInteraction );
}


void uiLabel::setPixmap( const ioPixmap& pixmap )
{
    body_->setPixmap( *pixmap.qpixmap() );
}


void uiLabel::setAlignment( Alignment::HPos hal )
{
    Alignment al( hal, Alignment::VCenter );
    body_->setAlignment( (Qt::AlignmentFlag)al.uiValue() );
}


Alignment::HPos uiLabel::alignment() const
{
    Alignment al;
    al.setUiValue( (int)body_->alignment() );
    return al.hPos();
}
