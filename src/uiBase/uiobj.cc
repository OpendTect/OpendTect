/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id: uiobj.cc,v 1.56 2003-11-07 12:22:00 bert Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uiobjbody.h"
#include "uimainwin.h"
#include "i_layoutitem.h"
#include "errh.h"
#include "timer.h"
#include "pixmap.h"
#include "color.h"

#include <qsettings.h> 

int uiObject::basefldsize_ = 10;


#define mBody_( imp_ )	dynamic_cast<uiObjectBody*>( imp_ )
#define mBody()		mBody_( body() )
#define mConstBody()	mBody_(const_cast<uiObject*>(this)->body())

//#define pbody()		static_cast<uiParentBody*>( body() )

void uiObjHandle::finalise()
    { if (body()) body()->finalise(); }

void uiObjHandle::clear()
    { if (body()) body()->clear(); }


uiParent::uiParent( const char* nm, uiParentBody* b )
    : uiObjHandle(nm, b)  				{}


void uiParent::addChild( uiObjHandle& child )
{
    mDynamicCastGet( uiObjHandle*, thisuiobj, this );
    if ( thisuiobj && child == thisuiobj ) return;
    if ( !body() )		{ pErrMsg("uiParent has no body!"); return; } 

    uiParentBody* b = dynamic_cast<uiParentBody*>( body() );
    if ( !b )			
	{ pErrMsg("uiParent has a body, but it's no uiParentBody"); return; } 

    b->addChild( child );
}


void uiParent::manageChld( uiObjHandle& child, uiObjectBody& bdy )
{
    if ( &child == static_cast<uiObjHandle*>(this) ) return;
    if ( !body() )		{ pErrMsg("uiParent has no body!"); return; } 

    uiParentBody* b = dynamic_cast<uiParentBody*>( body() );
    if ( !b )	return;

    b->manageChld( child, bdy );
}


void uiParent::attachChild ( constraintType tp, uiObject* child,
			     uiObject* other, int margin, bool reciprocal )
{
    if ( child == static_cast<uiObjHandle*>(this) ) return;
    if ( !body() )		{ pErrMsg("uiParent has no body!"); return; } 

    uiParentBody* b = dynamic_cast<uiParentBody*>( body() );
    if ( !b )			
	{ pErrMsg("uiParent has a body, but it's no uiParentBody"); return; } 

    b->attachChild ( tp, child, other, margin, reciprocal );
}


void uiParent::storePosition()
{
    QWidget* widget = body()->qwidget();
    if ( !widget ) { pErrMsg("no qwidget!"); return; }

    QSettings settings;

    BufferString key( "/dTect/geometry/" );
    key += name();

    QPoint p = widget->pos();

    QString k(key); k += "x";
    settings.writeEntry( k, p.x() );

    k = key; k += "y";
    settings.writeEntry( k, p.y() );

    QSize s = widget->size();

    k = key; k += "width";
    settings.writeEntry( k, s.width() );

    k = key; k += "height";
    settings.writeEntry( k, s.height() );
}

const Color& uiParent::backgroundColor() const
{
    return mainObject() ? mainObject()->backgroundColor() : *new Color();
}


void uiParent::restorePosition()
{
    QWidget* widget = body()->qwidget();
    if ( !widget ) { pErrMsg("no qwidget!"); return; }

    QSettings settings;

    BufferString key( "/dTect/geometry/" );
    key += name();

    QString k(key); k += "width";
    int w = settings.readNumEntry( k, -1 );

    k = key; k += "height";
    int h = settings.readNumEntry( k, -1 );

    if ( w >= 0 && h >= 0 )
	widget->resize( QSize(w,h) );

    k = key; k += "x";
    int x = settings.readNumEntry( k, -1 );

    k = key; k += "y";
    int y = settings.readNumEntry( k, -1 );

    if ( x >= 0 && y >= 0 )
	widget->move( QPoint(x,y) );
}


uiParentBody* uiParent::pbody()
{
    return dynamic_cast<uiParentBody*>( body() );
}


void uiParentBody::finaliseChildren()
{
    if ( !finalised_ )
    {
	finalised_= true;
	for ( int idx=0; idx<children.size(); idx++ )
	    children[idx]->finalise();
    }
}


void uiParentBody::clearChildren()
{
    for ( int idx=0; idx<children.size(); idx++ )
	children[idx]->clear();
}



uiObject::uiObject( uiParent* p, const char* nm )
    : uiObjHandle( nm, 0 )
    , finaliseStart(this)
    , finaliseDone(this)
    , setGeometry(this)
    , closed(this)
    , parent_( p )				
{ 
    if ( p ) p->addChild( *this );  
}

uiObject::uiObject( uiParent* p, const char* nm, uiObjectBody& b )
    : uiObjHandle( nm, &b )
    , finaliseStart(this)
    , finaliseDone(this)
    , setGeometry(this)
    , closed(this)
    , parent_( p )				
{ 
    if ( p ) p->manageChld( *this, b );  
}

void uiObject::setHSzPol( SzPolicy p )
    { mBody()->setHSzPol(p); }

void uiObject::setVSzPol( SzPolicy p )
    { mBody()->setVSzPol(p); }


uiObject::SzPolicy uiObject::szPol(bool hor) const
    { return mConstBody()->szPol(hor); }

void uiObject::setToolTip(const char* t)
    { mBody()->setToolTip(t); }


void uiObject::enableToolTips(bool yn)	{ uiObjectBody::enableToolTips(yn); }


bool uiObject::toolTipsEnabled() 
    { return uiObjectBody::toolTipsEnabled(); }


void uiObject::display( bool yn, bool shrink, bool maximise )	
{ 
    finalise();
    mBody()->display(yn,shrink,maximise); 
}

void uiObject::setFocus()			
    { mBody()->uisetFocus(); }
    
bool uiObject::hasFocus() const			
    { return mConstBody()->uihasFocus(); }


const Color& uiObject::backgroundColor() const	
    { return mConstBody()->uibackgroundColor(); }


void uiObject::setBackgroundColor(const Color& col)
    { mBody()->uisetBackgroundColor(col); }


void uiObject::setBackgroundPixmap( const char* img[] )
    { mBody()->uisetBackgroundPixmap( img ); }

void uiObject::setBackgroundPixmap( const ioPixmap& pm )
    { mBody()->uisetBackgroundPixmap( pm ); }

void uiObject::setSensitive(bool yn)	
    { mBody()->uisetSensitive(yn); }


bool uiObject::sensitive() const
    { return mConstBody()->uisensitive(); }


int uiObject::prefHNrPics() const
    { return mConstBody()->prefHNrPics(); }


void uiObject::setPrefWidth( int w )
    { mBody()->setPrefWidth(w); }


void uiObject::setPrefWidthInChar( float w )
     { mBody()->setPrefWidthInChar(w); }


int uiObject::prefVNrPics() const
    { return mConstBody()->prefVNrPics(); }


void uiObject::setPrefHeight( int h )
    { mBody()->setPrefHeight(h); }


void uiObject::setPrefHeightInChar( float h )
     {mBody()->setPrefHeightInChar(h);}


void uiObject::setStretch( int hor, int ver )
     {mBody()->setStretch(hor,ver); }


void uiObject::attach ( constraintType tp, int margin )
    { mBody()->attach(tp, (uiObject*)0, margin); }

void uiObject::attach ( constraintType tp, uiObject* other, int margin,
			bool reciprocal )
    { mBody()->attach(tp, other, margin, reciprocal); }

void uiObject::attach ( constraintType tp, uiParent* other, int margin,
			bool reciprocal )
    { mBody()->attach(tp, other, margin, reciprocal); }

/*!
    Moves the \a second widget around the ring of focus widgets so
    that keyboard focus moves from the \a first widget to the \a
    second widget when the Tab key is pressed.

    Note that since the tab order of the \a second widget is changed,
    you should order a chain like this:

    \code
        setTabOrder( a, b ); // a to b
        setTabOrder( b, c ); // a to b to c
        setTabOrder( c, d ); // a to b to c to d
    \endcode

    \e not like this:

    \code
        setTabOrder( c, d ); // c to d   WRONG
        setTabOrder( a, b ); // a to b AND c to d
        setTabOrder( b, c ); // a to b to c, but not c to d
    \endcode

    If \a first or \a second has a focus proxy, setTabOrder()
    correctly substitutes the proxy.
*/
void uiObject::setTabOrder( uiObject* first, uiObject* second )
{
    QWidget::setTabOrder( first->body()->qwidget(), second->body()->qwidget() );
}



void uiObject::setFont( const uiFont& f )
    { mBody()->uisetFont(f); }


const uiFont* uiObject::font() const
    { return mConstBody()->uifont(); }


uiSize uiObject::actualsize( bool include_border ) const
    { return mConstBody()->actualsize( include_border ); }


void uiObject::setCaption( const char* c )
    { mBody()->uisetCaption(c); }



void uiObject::triggerSetGeometry( const i_LayoutItem* mylayout, uiRect& geom )
    { if ( mylayout == mBody()->layoutItem() ) setGeometry.trigger(geom); }   


void uiObject::reDraw( bool deep )
    { mBody()->reDraw( deep ); }


uiMainWin* uiObject::mainwin()
{
    uiParent* par = parent();
    if ( !par )
    {
	mDynamicCastGet(uiMainWin*,mw,this)
	return mw;
    }

    return par->mainwin();
}


void uiObject::close()
{
    if ( body() && body()->qwidget() )
	body()->qwidget()->close();
}


int uiObject::baseFldSize()	{ return basefldsize_; }

