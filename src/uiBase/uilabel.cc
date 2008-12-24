/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          7/9/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uilabel.cc,v 1.13 2008-12-24 05:49:25 cvsnanne Exp $";


#include "uilabel.h"
#include "uiobjbody.h"

#include <qlabel.h> 


class uiLabelBody : public uiObjBodyImpl<uiLabel,QLabel>
{
public:

                        uiLabelBody( uiLabel& handle, uiParent* parnt,
				     const char* txt )
			    : uiObjBodyImpl<uiLabel,QLabel>(handle,parnt,txt)
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
}


const char* uiLabel::text() const
{ return mQStringToConstChar(body_->text()); }


void uiLabel::setAlignment( OD::Alignment al )
{ 
    body_->setAlignment( (Qt::Alignment)al );
}


OD::Alignment uiLabel::alignment() const
{
    Qt::Alignment qtal =  body_->alignment();
    if ( qtal.testFlag(Qt::AlignRight) )
	return OD::AlignRight;
    else if ( qtal.testFlag(Qt::AlignHCenter) )
	return OD::AlignHCenter;

    return OD::AlignLeft;
}
