/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          7/9/2000
________________________________________________________________________

-*/


#include <uilabel.h>
#include <qlabel.h> 
#include <uiobjbody.h>


class uiLabelBody : public uiObjBodyImpl<uiLabel,QLabel>
{
public:

                        uiLabelBody(uiLabel& handle, uiParent* parnt,
				    const char* txt )
			    : uiObjBodyImpl<uiLabel,QLabel>( handle, parnt,txt )
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


uiLabel::uiLabel(uiParent* parnt,const char* txt, uiObject* buddy)
    : uiObject( parnt, txt, mkbody(parnt,txt) )
{
    setText(txt);

    if ( buddy ) 
    {
	body_->setBuddy( buddy->body()->qwidget() );
	buddy->attach( rightOf, this );
    }
    setStretch( 0, 0 );
}

uiLabelBody& uiLabel::mkbody(uiParent* parnt,const char* txt)
{ 
    body_= new uiLabelBody(*this,parnt,txt);
    return *body_; 
}

void uiLabel::setText( const char* txt )
{ 
    body_->setText( QString( txt ) ); 
}

const char* uiLabel::text()
    { return body_->text(); }

