/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id: uiobj.cc,v 1.22 2002-01-07 13:17:01 arend Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uiobjbody.h"
#include "uigroup.h"
#include "uimainwin.h"
#include "uibuttongroup.h"
#include "i_layout.h"
#include "i_layoutitem.h"
#include "errh.h"
#include "timer.h"

#include <qpalette.h> 
#include <qtooltip.h> 
#include <qfontmetrics.h> 

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
    mDynamicCastGet( uiObjHandle*, thisuiobj, this );
    if ( thisuiobj && child == thisuiobj ) return;
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


void uiObject::display( bool yn, bool shrink )	{ mBody()->display(yn,shrink); }

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

uiMainWin* uiObject::mainwin()
{
    uiParent* par = parent();
    if ( !par )
    {
	mDynamicCastGet(uiMainWin*,mw,this)
	return mw;
    }

    while ( 1 )
    {
	mDynamicCastGet(uiMainWin*,mw,par)
	if ( mw ) return mw;

	mDynamicCastGet(uiObject*,uiobj,par);
	if ( !uiobj ) return 0;

	par = uiobj->parent();
	if ( !par ) return 0;
    }
    return 0;
}


uiObjectBody::uiObjectBody( uiParent* parnt )
    : uiBody()
    , layoutItem_( 0 )
    , parent_( parnt ? mParntBody(parnt) : 0  )
    , font_( 0 )
    , hStretch( mUndefIntVal  )
    , vStretch(  mUndefIntVal )
    , is_hidden( false )
    , finalised( false )
    , display_( true )
    , popped_up_( false )
    , pref_width_( 0 )
    , pref_height_( 0 )
    , pref_width_set( - 1 )
    , pref_char_width( -1 )
    , pref_height_set( -1 )
    , pref_char_height( -1 )
    , pref_width_hint( 0 )
    , pref_height_hint( 0 )
    , fnt_hgt( 0 )
    , fnt_wdt( 0 )
    , fnt_maxwdt( 0 )
    , fm( 0 )
#ifdef USE_DISPLAY_TIMER
    , displTim( *new Timer("Display timer"))
{ 
    displTim.tick.notify(mCB(this,uiObjectBody,doDisplay));
}
#else
{}
#endif

uiObjectBody::~uiObjectBody() 
{
#ifdef USE_DISPLAY_TIMER
    delete &displTim;
#endif
    delete fm;
}


void uiObjectBody::display( bool yn, bool shrink )
{
    popped_up_ = true;

    display_ = yn;

    if( shrink )
    {
	pref_width_  = 0;
	pref_height_ = 0;

	is_hidden = true;

	qwidget()->hide();
    }
    else
    {
#ifdef USE_DISPLAY_TIMER
	if( displTim.isActive() ) displTim.stop();
	displTim.start( 1, true );
#else
	doDisplay(0);
#endif
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
		int sz = prefHNrPics();
		sz = prefVNrPics();

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

void uiObjectBody::fontchanged()
{
    fnt_hgt=0;  fnt_wdt=0; fnt_maxwdt=0;
    delete fm;
    fm=0;

    pref_width_hint=0;
    pref_height_hint=0;
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
#define mChkLayoutItm() if(!layoutItem_) { pErrMsg("No layoutItem"); return 0; }
#else
#define mChkLayoutItm() if(!layoutItem_) { return 0; } 
#endif

void uiObjectBody::getSzHint()
{
    if( pref_width_hint && pref_height_hint ) return;
    if( pref_width_hint || pref_height_hint ) 
	{ pErrMsg("Only 1 defined size.."); }

    uiSize sh = layoutItem_->prefSize();

    pref_width_hint = sh.width();
    pref_height_hint = sh.height();
}

int uiObjectBody::prefHNrPics() const
{
/*
    if( !popped_up_ ) 
	{ pErrMsg("Huh? asked for prefHNrPics but not popped."); }
*/
    const_cast<uiObjectBody*>(this)->popped_up_ = true;

    if( pref_width_ <= 0 )
    {
	if( is_hidden ) 
	    { pErrMsg("Cannot calculate preferred size when hidden"); return 0;}
	mChkLayoutItm();
	if( pref_width_set >= 0 ) 
	    { const_cast<uiObjectBody*>(this)->pref_width_ = pref_width_set; }
	else if( pref_char_width >= 0 ) 
	{
	    int fw = fontWdt();
	    if( !fw ){ pErrMsg("Font has 0 width."); return 0; }

	    const_cast<uiObjectBody*>(this)->pref_width_ =
					     mNINT( pref_char_width * fw ); 
	}
	else
	{ 
	    const_cast<uiObjectBody*>(this)->getSzHint();
	    const_cast<uiObjectBody*>(this)->pref_width_ =
		    pref_width_hint;
	}
    }
    return pref_width_;
}


int uiObjectBody::prefVNrPics() const
{
/* 
    if( !popped_up_ ) 
	{ pErrMsg("Huh? asked for prefVNrPics but not popped."); }
*/
    const_cast<uiObjectBody*>(this)->popped_up_ = true;

    if( pref_height_ <= 0 )
    {
	if( is_hidden ) 
	    { pErrMsg("Cannot calculate preferred size when hidden"); return 0;}

	mChkLayoutItm();
	if( pref_height_set >= 0 ) 
	    { const_cast<uiObjectBody*>(this)->pref_height_= pref_height_set;}
	else if( pref_char_height >= 0 ) 
	{
	    int fh = fontHgt();
	    if( !fh ){ pErrMsg("Font has 0 height."); return 0; }

	    const_cast<uiObjectBody*>(this)->pref_height_ =
					    mNINT( pref_char_height * fh ); 
	}
	else
	{ 
	    const_cast<uiObjectBody*>(this)->getSzHint();
	    const_cast<uiObjectBody*>(this)->pref_height_ = pref_height_hint;

	    if( isSingleLine() && parent_ )
	    {
		int min_height =  parent_->minTextWidgetHeight();
		if( min_height >= 0  && pref_height_ <= min_height ) 
		    const_cast<uiObjectBody*>(this)->pref_height_ =
								    min_height;
	    }
	}
    }

    return pref_height_;
}


uiSize uiObjectBody::actualSize( bool include_border ) const
{
    mChkLayoutItm();
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
    if( popped_up_ ) 
	{ pErrMsg("Cannot attach when already popped up."); }
    parent_->attachChild( tp, &uiObjHandle(), other, margin );
}

const uiFont* uiObjectBody::uifont() const
{
    if( !font_ )
    { 
	const_cast<uiObjectBody*>(this)->font_ = 
					&uiFontList::get(className(*this)); 
	//const_cast<uiObjectBody*>(this)->qwidget()->setFont( font_->qFont() );
    }

    return font_;
}


void uiObjectBody::uisetFont( const uiFont& f )
{
    font_ = &f;
    qwidget()->setFont( font_->qFont() );
    parent_->setMinTextWidgetHeight();
}

int uiObjectBody::fontWdtFor( const char* str) const
{
    gtFntWdtHgt();
    if( !fm ) return 0;
    return fm->width( QString( str ) );
}

void uiObjectBody::gtFntWdtHgt() const
{
    if( !fnt_hgt || !fnt_wdt || !fnt_maxwdt || !fm )
    {
	if( fm ) delete fm;
	const_cast<uiObjectBody*>(this)->fm =
			     new QFontMetrics( qwidget()->font() );

	const_cast<uiObjectBody*>(this)->fnt_hgt = fm->lineSpacing() + 2;
	const_cast<uiObjectBody*>(this)->fnt_wdt = fm->width(QChar('x'));
	const_cast<uiObjectBody*>(this)->fnt_maxwdt = fm->maxWidth();
    }

#ifdef __debug__

    if( fnt_hgt<0 || fnt_hgt>100 )
    { 
	pErrMsg("Font heigt no good. Taking 25."); 
	const_cast<uiObjectBody*>(this)->fnt_hgt = 25;
    }
    if( fnt_wdt<0 || fnt_wdt>100 )
    { 
	pErrMsg("Font width no good. Taking 10."); 
	const_cast<uiObjectBody*>(this)->fnt_wdt = 10;
    }
    if( fnt_maxwdt<0 || fnt_maxwdt>100 )
    { 
	pErrMsg("Font maxwidth no good. Taking 15."); 
	const_cast<uiObjectBody*>(this)->fnt_maxwdt = 15;
    }

#endif
}
