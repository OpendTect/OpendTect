/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2008
________________________________________________________________________

-*/

#include "uiscrollarea.h"

#include "uiobjbodyimpl.h"
#include "q_uiimpl.h"

#include <QScrollArea>
#include <QScrollBar>

mUseQtnamespace

class ODScrollArea : public uiObjBodyImpl<uiScrollArea,QScrollArea>
{
public:
			ODScrollArea(uiScrollArea&,uiParent*,const char*);
			~ODScrollArea();

    QSize		sizeHint() const override;

protected:

    void		resizeEvent(QResizeEvent*) override;
};


ODScrollArea::ODScrollArea( uiScrollArea& hndle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiScrollArea,QScrollArea>(hndle,p,nm)
{}

ODScrollArea::~ODScrollArea()
{}


QSize ODScrollArea::sizeHint() const
{
    QSize qsz;
    if ( handle_.object_ )
    {
	if ( handle_.limitheight_ )
	{
	    const QScrollBar* hsb = horizontalScrollBar();
	    const int barheight = hsb ? hsb->height() : 0;
	    qsz.setHeight( handle_.object_->height()+barheight );
	}

	if ( handle_.limitwidth_ )
	{
	    const QScrollBar* vsb = verticalScrollBar();
	    const int barwidth = vsb ? vsb->height() : 0;
	    qsz.setWidth( handle_.object_->width()+barwidth );
	}
    }

    return qsz;
}


void ODScrollArea::resizeEvent( QResizeEvent* ev )
{
    QScrollArea::resizeEvent( ev );
}


uiScrollArea::uiScrollArea( uiParent* p, const char* nm )
    : uiObject(p,nm,mkbody(p,nm))
{
    setStretch( 2, 2 );
}


ODScrollArea& uiScrollArea::mkbody( uiParent* p, const char* nm )
{
    body_ = new ODScrollArea( *this, p, nm );
    return *body_;
}


uiScrollArea::~uiScrollArea()
{ delete body_; }


void uiScrollArea::setObject( uiObject* obj )
{
    object_ = obj;
    if ( obj )
	body_->setWidget( obj->qwidget() );
}


uiObject* uiScrollArea::getObject()
{
    if ( !object_ )
	return nullptr;

    body_->takeWidget();
    return object_;
}


void uiScrollArea::setObjectResizable( bool yn )
{
    body_->setWidgetResizable( yn );
}
