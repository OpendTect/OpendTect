/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          7/9/2000
________________________________________________________________________

-*/


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
			    const char* txt = text();
			    while( txt && *txt )
				{ if( *txt == '\n' ) nrl++; txt++; }

			    return nrl; 
			}

};


uiLabel::uiLabel( uiParent* p, const char* txt )
    : uiObject(p,txt,mkbody(p,txt))
{
    setText(txt);
    setStretch( 0, 0 );
}


uiLabel::uiLabel( uiParent* p, const char* txt, uiGroup* grp )
    : uiObject(p,txt,mkbody(p,txt))
{
    setText(txt);

    uiObject* buddy = grp ? grp->attachObj() : 0;
    if ( buddy )
    {
	body_->setBuddy( buddy->body()->qwidget() );
	buddy->attach( rightOf, this );
    }
    setStretch( 0, 0 );
}


uiLabel::uiLabel( uiParent* p, const char* txt, uiObject* buddy )
    : uiObject(p,txt,mkbody(p,txt))
{
    setText(txt);

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
{ return body_->text(); }


void uiLabel::setAlignment( int al )
{ 
    int align = body_->alignment();
    align &= Qt::AlignVertical_Mask;
    
    align |= al;

    body_->setAlignment( align );
}


int uiLabel::alignment() const
{
    int align = body_->alignment();
    return align & Qt::AlignHorizontal_Mask;
}
