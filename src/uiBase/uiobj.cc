/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id: uiobj.cc,v 1.7 2001-06-08 15:59:43 bert Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "i_layout.h"
#include "i_qobjwrap.h"
#include "errh.h"
#include "timer.h"

#include <qpalette.h> 
#include <qtooltip.h> 



uiObject::uiObject( uiParent* parnt, const char* nm )
    : UserIDObject( nm )
    , parent_( parnt )
    , horStretch( 0 )
    , verStretch( 0 )
    , mLayoutItm( 0 )
    , isHidden( false )
    , pref_width( -1 )
    , pref_char_width( -1 )
    , pref_height( -1 )
    , pref_char_height( -1 )
    , cached_pref_width( 0 )
    , cached_pref_height( 0 )
    , font_( 0 )
    , finalised( false )
{ }

uiObject::~uiObject() 
{ }


void uiObject::show()
{
    isHidden = false;
    finalise();
    qWidget().show();
}

void uiObject::hide()
{
    if( !isHidden )
    {
	cached_pref_width  = preferredWidth();
	cached_pref_height = preferredHeight();
	isHidden = true; // not before call to preferredXX !!
    }

    qWidget().hide();
}

void uiObject::setFocus()
{ 
    qWidget().setFocus();
}

void uiObject::setSensitive( bool yn )
{
    qWidget().setEnabled( yn );
}


bool uiObject::sensitive() const
{
    return qWidget().isEnabled();
}

void uiObject::forceRedraw_( bool deep )
{
    qWidget().update();
}


Color uiObject::backgroundColor() const
{
    return Color( qWidget().backgroundColor().rgb() );
}


void uiObject::setBackgroundColor( const Color& c )
{
    QPalette p = qWidget().palette();
    p.setColor( QColorGroup::Background, QColor( QRgb( c.rgb() ))  );
    qWidget().setPalette( p );
}


int uiObject::preferredWidth() const
{   // Also look at uiComboBox::preferredWidth() when changing this method.

    mChkmLayout();
    if( isHidden ) return cached_pref_width;
    if( pref_width >= 0 ) return pref_width;
    if( pref_char_width >= 0 ) 
    {
	if( !font() ){ pErrMsg("uiObject has no font!"); return 0; }
 
#ifdef EXTENSIVE_DEBUG
	int ret_val = pref_char_width * font()->avgWidth();
	BufferString msg;
	msg += "Preferred width of";
	msg += name();
	msg += " = ";
	msg+= ret_val;
	pErrMsg(msg);
	return ret_val;
#else
	return mNINT( pref_char_width * (float)font()->avgWidth() ); 
#endif
    }

    return mLayoutItm->mQLayoutItem().sizeHint().width(); 
}


int uiObject::horAlign() const
{
    mChkmLayout();
    return mLayoutItm->i_LayoutItem::horAlign();
}


int uiObject::horCentre() const
//! \return -1 if error
{
    mChkmLayout();
    return mLayoutItm->i_LayoutItem::horCentre();
}


int uiObject::preferredHeight() const
{ 
    mChkmLayout();
    if( pref_height >= 0 ) return pref_height;
    if( pref_char_height >= 0 ) 
	{ return mNINT( pref_char_height * (float)font()->height() ); }

    int prfHgt = isHidden ? cached_pref_height 
			  : mLayoutItm->mQLayoutItem().sizeHint().height(); 

    if( isSingleLine() )
    {
	int min_height =  mLayoutMngr() ? mLayoutMngr()->minTxtWidgHgt() : 0;
	if( min_height >= 0  && prfHgt <= min_height ) 
	    return min_height;
    }

    return prfHgt;

}


uiSize uiObject::minimumSize() const
/*! \return minimum size of widget; (-1,-1) if failure. 
     Used by dGB's layout manager 					*/
{
    mChkmLayout();
    QSize s = mLayoutItm->i_LayoutItem::minimumSize();
    return uiSize( s.width(), s.height() );
}


uiSize uiObject::actualSize( bool include_border ) const
{
    mChkmLayout();
    return mLayoutItm->actualSize( include_border );
}


int uiObject::minimumTextWidgetHeight() const
{
    const i_LayoutMngr* mgr = mLayoutMngr();
    return mgr ? mgr->minTxtWidgHgt() : 30;
}


void uiObject::setToolTip( const char* txt )
{
    QToolTip::add( &qWidget(), txt );
}


void uiObject::enableToolTips( bool yn )
{
    QToolTip::setEnabled( yn );
}


bool uiObject::toolTipsEnabled()
{
    return QToolTip::enabled();
}


void uiObject::setCaption( const char* str )
{
    qWidget().setCaption( QString( str ) );
}


int uiObject::borderSpace() const
{
    return mLayoutMngr() ? mLayoutMngr()->borderSpace() : 10;
}


/*!
    attaches to parent if other=0
*/
bool uiObject::attach ( constraintType tp, uiObject* other, int margin )
{
    if ( prntLayoutMngr() ) 
	return prntLayoutMngr()->attach ( tp, qWidget(), 
					  other ? &other->qWidget() : 0,
					  margin );
    else
	return false;
}

const uiFont* uiObject::font() const
{
    if( !font_ )
    { const_cast<uiObject*>(this)->font_ = &uiFontList::get(className(*this)); }

    return font_;
}

void uiObject::setFont( const uiFont& f )
{
    font_ = &f;
    qWidget().setFont( font_->qFont() );
    mLayoutMngr()->setMinTxtWidgHgt( 0 );
}
