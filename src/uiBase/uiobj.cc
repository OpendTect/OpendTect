/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id: uiobj.cc,v 1.13 2001-10-05 13:20:15 arend Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uiobjbody.h"
#include "uigroup.h"
#include "uibuttongroup.h"
#include "i_layout.h"
#include "i_layoutitem.h"
#include "errh.h"
#include "timer.h"

#include <qpalette.h> 
#include <qtooltip.h> 

#define mBody_( imp_ )	dynamic_cast<uiObjectBody*>( imp_ )
#define mBody()		mBody_( body() )
#define mConstBody()	mBody_(const_cast<uiObject*>(this)->body())

#define pbody()		dynamic_cast<uiParentBody*>( body() )
//#define pbody()		static_cast<uiParentBody*>( body() )
#define mParntBody( p ) dynamic_cast<uiParentBody*>( p->body() )


void uiObjHandle::finalise()
    { if (body()) body()->finalise(); }

void uiObjHandle::clear()
    { if (body()) body()->clear(); }


uiParent::uiParent( const char* nm, uiParentBody* b )
    : uiObjHandle(nm, b)  				{}


void uiParent::addChild( uiObjHandle& child )
{
    if ( child == static_cast<uiObjHandle*>(this) ) return;
    if ( !body() )		{ pErrMsg("uiParent has no body!"); return; } 

    uiParentBody* b = pbody();
    if ( !b )			
	{ pErrMsg("uiParent has a body, but it's no uiParentBody"); return; } 

    b->addChild( child );
}


void uiParent::manageChld( uiObjHandle& child, uiObjectBody& bdy )
{
    if ( &child == static_cast<uiObjHandle*>(this) ) return;
    if ( !body() )		{ pErrMsg("uiParent has no body!"); return; } 

    uiParentBody* b = pbody();
    if ( !b )			
	{ pErrMsg("uiParent has a body, but it's no uiParentBody"); return; } 

    b->manageChld( child, bdy );
}


void uiParent::attachChild ( constraintType tp, uiObject* child,
			     uiObject* other, int margin )
{
    if ( child == static_cast<uiObjHandle*>(this) ) return;
    if ( !body() )		{ pErrMsg("uiParent has no body!"); return; } 

    uiParentBody* b = pbody();
    if ( !b )			
	{ pErrMsg("uiParent has a body, but it's no uiParentBody"); return; } 

    b->attachChild ( tp, child, other, margin );
}


int uiParent::minTextWidgetHeight() const
{
    const uiParentBody* b = dynamic_cast<const uiParentBody*>( body() );
    return b->minTextWidgetHeight();
}


void uiParentBody::finaliseChildren()
{
    if(!finalised)
    {
	finalised= true;
	for( int idx=0; idx<children.size(); idx++ )
	    children[idx]->finalise();
    }
}


void uiParentBody::clearChildren()
{
    for( int idx=0; idx<children.size(); idx++ )
	children[idx]->clear();
}

uiObject::uiObject( uiParent* p, const char* nm )
    : uiObjHandle( nm, 0 )
    , finalising(this)
    , setGeometry(this)
    , parent_( p )				
{ 
    if ( p ) p->addChild( *this );  
}

uiObject::uiObject( uiParent* p, const char* nm, uiObjectBody& b )
    : uiObjHandle( nm, &b )
    , finalising(this)
    , setGeometry(this)
    , parent_( p )				
{ 
    if ( p ) p->manageChld( *this, b );  
}


void uiObject::setToolTip(const char* t)
    { mBody()->setToolTip(t); }


void uiObject::enableToolTips(bool yn)	{ uiObjectBody::enableToolTips(yn); }


bool uiObject::toolTipsEnabled() 
    { return uiObjectBody::toolTipsEnabled(); }


void uiObject::display( bool yn = true, bool shrink=false )
						{ mBody()->display(yn,shrink); }

void uiObject::setFocus()			{ mBody()->uisetFocus();}



Color uiObject::backgroundColor() const	
    { return mConstBody()->uibackgroundColor(); }


void uiObject::setBackgroundColor(const Color& col)
    { mBody()->uisetBackgroundColor(col); }


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
    { mBody()->attach(tp, 0, margin); }

void uiObject::attach ( constraintType tp, uiObject* other, int margin )
    { mBody()->attach(tp, other, margin); }

void uiObject::attach ( constraintType tp, uiGroup* other, int margin )
    { mBody()->attach(tp, other->uiObj(), margin); }

void uiObject::attach ( constraintType tp, uiButtonGroup* other, int margin )
    { mBody()->attach(tp, other->uiObj(), margin); }


void uiObject::setFont( const uiFont& f )
    { mBody()->uisetFont(f); }


const uiFont* uiObject::font() const
    { return mConstBody()->uifont(); }


uiSize uiObject::actualSize( bool include_border ) const
    { return mConstBody()->actualSize( include_border ); }


void uiObject::setCaption( const char* c )
    { mBody()->uisetCaption(c); }



void uiObject::triggerSetGeometry( const i_LayoutItem* mylayout, uiRect& geom )
    { if ( mylayout == mBody()->layoutItem() ) setGeometry.trigger(geom); }   

void uiObject::reDraw( bool deep )
    { mBody()->reDraw( deep ); }


uiObjectBody::uiObjectBody( uiParent* parnt )
    : uiBody()
    , hStretch( mUndefIntVal  )
    , vStretch(  mUndefIntVal )
    , layoutItem_( 0 )
    , is_hidden( false )
    , display_( true )
    , finalised( false )
    , pref_width( - 1 )
    , pref_char_width( -1 )
    , pref_height( -1 )
    , pref_char_height( -1 )
    , cached_pref_width( 0 )
    , cached_pref_height( 0 )
    , parent_( parnt ? mParntBody(parnt) : 0  )
    , font_( 0 )
    , displTim( *new Timer("Display timer"))
{ 
    displTim.tick.notify(mCB(this,uiObjectBody,doDisplay));
}

uiObjectBody::~uiObjectBody() 
{ 
    delete &displTim;
}


void uiObjectBody::display( bool yn, bool shrink )
{
    display_ = yn;

    if( shrink )
    {
	cached_pref_width  = 0;
	cached_pref_height = 0;

	is_hidden = true;

	qwidget()->hide();
    }
    else
    {
	if( displTim.isActive() ) displTim.stop();
	displTim.start( 1, true );
    }
}

void uiObjectBody::doDisplay(CallBacker*)
{
    if( finalised )
    {
	if( display_ )
	{
	    is_hidden = false;
	    qwidget()->show();
	}
	else
	{
	    if( !is_hidden )
	    {
		cached_pref_width  = prefHNrPics();
		cached_pref_height = prefVNrPics();

		is_hidden = true;

		qwidget()->hide();
	    }
	}
    }
}

void uiObjectBody::uisetFocus()
{ 
    qwidget()->setFocus();
}

void uiObjectBody::uisetSensitive( bool yn )
{
    qwidget()->setEnabled( yn );
}


bool uiObjectBody::uisensitive() const
{
    return qwidget()->isEnabled();
}

void uiObjectBody::reDraw( bool deep )
{
    qwidget()->update();
}


Color uiObjectBody::uibackgroundColor() const
{
    return Color( qwidget()->backgroundColor().rgb() );
}


void uiObjectBody::uisetBackgroundColor( const Color& c )
{
    QPalette p = qwidget()->palette();
    p.setColor( QColorGroup::Background, QColor( QRgb( c.rgb() ))  );
    qwidget()->setPalette( p );
}


#ifdef __debug__ 
#define mChkmLayout()   if(!layoutItem_) { pErrMsg("No layoutItem"); return 0; }
#else
#define mChkmLayout()   if(!layoutItem_) { return 0; } 
#endif


int uiObjectBody::prefHNrPics() const
{   // Also look at uiComboBox::prefHNrPics() when changing this method.

    mChkmLayout();
    if( pref_width >= 0 ) return pref_width;
    if( pref_char_width >= 0 ) 
    {
	if( !uifont() ){ pErrMsg("uiObjectBody has no uifont!"); return 0; }
 
#ifdef EXTENSIVE_DEBUG
	int ret_val = pref_char_width * uifont()->avgWidth();
	BufferString msg;
	msg += "Preferred width of";
	msg += name();
	msg += " = ";
	msg+= ret_val;
	pErrMsg(msg);
	return ret_val;
#else
	return mNINT( pref_char_width * (float)uifont()->avgWidth() ); 
#endif
    }

    if( is_hidden ) return cached_pref_width;
    return layoutItem_->sizeHint().width(); 
}


int uiObjectBody::prefVNrPics() const
{ 
    mChkmLayout();
    if( pref_height >= 0 ) return pref_height;
    if( pref_char_height >= 0 ) 
	{ return mNINT( pref_char_height * (float)uifont()->height() ); }

    int prfvnp = is_hidden ? cached_pref_height 
			   : layoutItem_->sizeHint().height(); 

    if( isSingleLine() && parent_ )
    {
	int min_height =  parent_->minTextWidgetHeight();
	if( min_height >= 0  && prfvnp <= min_height ) 
	    return min_height;
    }

    return prfvnp;

}



uiSize uiObjectBody::actualSize( bool include_border ) const
{
    mChkmLayout();
    return layoutItem_->actualSize( include_border );
}


void uiObjectBody::setToolTip( const char* txt )
{
    QToolTip::add( qwidget(), txt );
}


void uiObjectBody::enableToolTips( bool yn )
{
    QToolTip::setEnabled( yn );
}


bool uiObjectBody::toolTipsEnabled()
    { return QToolTip::enabled(); }


void uiObjectBody::uisetCaption( const char* str )
    { qwidget()->setCaption( QString( str ) ); }

i_LayoutItem* uiObjectBody::mkLayoutItem_( i_LayoutMngr& mngr )
    { return new i_uiLayoutItem( mngr , *this ); }



/*!
    attaches to parent if other=0
*/
void uiObjectBody::attach ( constraintType tp, uiObject* other, int margin )
{
//    parent_->attachChild( tp, this, other, margin );
    parent_->attachChild( tp, &uiObjHandle(), other, margin );
}

const uiFont* uiObjectBody::uifont() const
{
    if( !font_ )
    { 
	const_cast<uiObjectBody*>(this)->font_ = 
					&uiFontList::get(className(*this)); 
    }

    return font_;
}

void uiObjectBody::uisetFont( const uiFont& f )
{
    font_ = &f;
    qwidget()->setFont( font_->qFont() );
    parent_->setMinTextWidgetHeight();
}
