/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id: uiobj.cc,v 1.51 2003-04-22 09:49:48 arend Exp $
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
#include "pixmap.h"

#include <qpalette.h> 
#include <qpixmap.h> 
#include <qtooltip.h> 
#include <qfontmetrics.h> 
#include <qsettings.h> 

int uiObject::basefldsize_ = 10;


#define mBody_( imp_ )	dynamic_cast<uiObjectBody*>( imp_ )
#define mBody()		mBody_( body() )
#define mConstBody()	mBody_(const_cast<uiObject*>(this)->body())

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

Color uiParent::backgroundColor() const
    { return mainObject() ? mainObject()->backgroundColor() : Color(); }


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
    , finalising(this)
    , setGeometry(this)
    , close(this)
    , parent_( p )				
{ 
    if ( p ) p->addChild( *this );  
}

uiObject::uiObject( uiParent* p, const char* nm, uiObjectBody& b )
    : uiObjHandle( nm, &b )
    , finalising(this)
    , setGeometry(this)
    , close(this)
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

void uiObject::setFocus()			{ mBody()->uisetFocus();}



Color uiObject::backgroundColor() const	
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

int uiObject::baseFldSize()	{ return basefldsize_; }


uiObjectBody::uiObjectBody( uiParent* parnt, const char* nm )
    : uiBody()
    , UserIDObject( nm )
    , layoutItem_( 0 )
    , parent_( parnt ? mParntBody(parnt) : 0  )
    , font_( 0 )
    , hStretch( mUndefIntVal  )
    , vStretch(  mUndefIntVal )
    , allowshrnk( false )
    , is_hidden( false )
    , finalised( false )
    , display_( true )
    , display_maximised( false )
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
    , hszpol( uiObject::undef )
    , vszpol( uiObject::undef )
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
    delete layoutItem_;
}


void uiObjectBody::display( bool yn, bool shrink, bool maximised )
{
    display_ = yn;
    display_maximised = maximised;

    if ( !display_ && shrink )

    {
	pref_width_  = 0;
	pref_height_ = 0;

	is_hidden = true;

	qwidget()->hide();
    }
    else
    {
#ifdef USE_DISPLAY_TIMER
	if ( displTim.isActive() ) displTim.stop();
	displTim.start( 1, true );
#else
	doDisplay(0);
#endif
    }
}

void uiObjectBody::doDisplay(CallBacker*)
{
    if ( !finalised ) finalise();


    if ( display_ )
    {
	is_hidden = false;

	if ( display_maximised )	qwidget()->showMaximized();
	else			qwidget()->show();
    }
    else
    {
	if ( !is_hidden )
	{
	    int sz = prefHNrPics();
	    sz = prefVNrPics();
	    is_hidden = true;

	    qwidget()->hide();
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
    qwidget()->setPaletteBackgroundColor( QColor( QRgb( c.rgb() ) ) );
}


void uiObjectBody::uisetBackgroundPixmap( const char* img[] )
{
    qwidget()->setPaletteBackgroundPixmap( QPixmap(img) );
}


void uiObjectBody::uisetBackgroundPixmap( const ioPixmap& pm )
{
    qwidget()->setPaletteBackgroundPixmap( *pm.Pixmap() );
}


#ifdef __debug__ 
#define mChkLayoutItm() if (!layoutItem_) { pErrMsg("No layoutItem"); return 0; }
#else
#define mChkLayoutItm() if (!layoutItem_) { return 0; } 
#endif

void uiObjectBody::getSzHint()
{
    if ( pref_width_hint && pref_height_hint ) return;
    if ( pref_width_hint || pref_height_hint ) 
	{ pErrMsg("Only 1 defined size.."); }

    uiSize sh = layoutItem_->prefSize();

    pref_width_hint = sh.hNrPics();
    pref_height_hint = sh.vNrPics();
}

int uiObjectBody::prefHNrPics() const
{
    if ( pref_width_ <= 0 )
    {
	if ( is_hidden )		{ return pref_width_; }
	mChkLayoutItm();

	if ( pref_width_set >= 0 ) 
	    { const_cast<uiObjectBody*>(this)->pref_width_ = pref_width_set; }
	else if ( pref_char_width >= 0 ) 
	{
	    int fw = fontWdt();
	    if ( !fw ){ pErrMsg("Font has 0 width."); return 0; }

	    const_cast<uiObjectBody*>(this)->pref_width_ =
					     mNINT( pref_char_width * fw ); 
	}
	else
	{ 
	    const_cast<uiObjectBody*>(this)->getSzHint();

	    const int baseFldSz = uiObject::baseFldSize();

	    int pwc=0;
	    bool var=false; 
	    switch( szPol(true) )
	    {
		case uiObject::small:    pwc=baseFldSz;     break;
		case uiObject::medium:   pwc=2*baseFldSz+1; break;
		case uiObject::wide:     pwc=4*baseFldSz+3; break;

		case uiObject::smallmax:
		case uiObject::smallvar: pwc=baseFldSz;     var=true; break;

		case uiObject::medmax:
		case uiObject::medvar:   pwc=2*baseFldSz+1; var=true; break;

		case uiObject::widemax:
		case uiObject::widevar:  pwc=4*baseFldSz+3; var=true; break;
	    }

	    if ( !pwc )
		const_cast<uiObjectBody*>(this)->pref_width_ = pref_width_hint;
	    else
	    {
		int fw = fontWdt();
		if ( !fw ){ pErrMsg("Font has 0 width."); }

		int pw = var ? mMAX(pref_width_hint, fw*pwc ) : fw*pwc ;
		const_cast<uiObjectBody*>(this)->pref_width_ = pw;
            }
	}
    }
    return pref_width_;
}


int uiObjectBody::prefVNrPics() const
{
    if ( pref_height_ <= 0 )
    {
	if ( is_hidden )		{ return pref_height_; }
	mChkLayoutItm();

	if ( pref_height_set >= 0 ) 
	    { const_cast<uiObjectBody*>(this)->pref_height_= pref_height_set;}
	else if ( pref_char_height >= 0 ) 
	{
	    int fh = fontHgt();
	    if ( !fh ){ pErrMsg("Font has 0 height."); return 0; }

	    const_cast<uiObjectBody*>(this)->pref_height_ =
					    mNINT( pref_char_height * fh ); 
	}
	else
	{ 
	    const_cast<uiObjectBody*>(this)->getSzHint();

	    if ( nrTxtLines() < 0 && szPol(false) == uiObject::undef  )
		const_cast<uiObjectBody*>(this)->pref_height_= pref_height_hint;
	    else 
	    {
		float lines = 1.51;
		if ( nrTxtLines() == 0 )	lines = 7;
		else if ( nrTxtLines() > 1 )	lines = nrTxtLines();

		const int baseFldSz = 1;

		bool var=false; 
		switch( szPol(false) )
		{
		    case uiObject::small:    lines=baseFldSz;     break;
		    case uiObject::medium:   lines=2*baseFldSz+1; break;
		    case uiObject::wide:     lines=4*baseFldSz+3; break;

		    case uiObject::smallmax:
		    case uiObject::smallvar:
			    lines=baseFldSz; var=true; break;

		    case uiObject::medmax:
		    case uiObject::medvar: 
			    lines=2*baseFldSz+1; var=true; break;

		    case uiObject::widemax:
		    case uiObject::widevar:
			    lines=4*baseFldSz+3; var=true; break;
		}


		int fh = fontHgt();
		if ( !fh ){ pErrMsg("Font has 0 height."); return 0; }

		int phc = mNINT( lines * fh);
		const_cast<uiObjectBody*>(this)->pref_height_=
				var ? mMAX(pref_height_hint, phc ) : phc ;
	    }
	}
    }

    return pref_height_;
}


uiSize uiObjectBody::actualsize( bool include_border ) const
{
    return layoutItem_->actualsize( include_border );
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
void uiObjectBody::attach ( constraintType tp, uiObject* other, int margin,
			    bool reciprocal )
{
//    parent_->attachChild( tp, this, other, margin );
    parent_->attachChild( tp, &uiObjHandle(), other, margin, reciprocal );
}

const uiFont* uiObjectBody::uifont() const
{
    if ( !font_ )
    { 
// TODO: implement new font handling. uiParent should have a get/set font 
//       and all children should use this font.
#if 0
	const_cast<uiObjectBody*>(this)->font_ = 
					&uiFontList::get(className(*this)); 
#else
	QFont qf( qwidget()->font() );
	const_cast<uiObjectBody*>(this)->font_ = &uiFontList::getFromQfnt(&qf); 
#endif
	const_cast<uiObjectBody*>(this)->qwidget()->setFont( font_->qFont() );
    }

    return font_;
}


void uiObjectBody::uisetFont( const uiFont& f )
{
    font_ = &f;
    qwidget()->setFont( font_->qFont() );
}

int uiObjectBody::fontWdtFor( const char* str) const
{
    gtFntWdtHgt();
    if ( !fm ) return 0;
    return fm->width( QString( str ) );
}

bool uiObjectBody::itemInited() const
{
    return layoutItem_ ? layoutItem_->inited() : false;
}


void uiObjectBody::gtFntWdtHgt() const
{
    if ( !fnt_hgt || !fnt_wdt || !fnt_maxwdt || !fm )
    {
	if ( fm )
	{
	    pErrMsg("Already have a fontmetrics. Deleting..."); 
	    delete fm;
	}
	const_cast<uiObjectBody*>(this)->fm =


// TODO: implement new font handling. uiParent should have a get/set font 
//       and all children should use this font.
#if 0
			     new QFontMetrics( uifont()->qFont() );
#else
			     new QFontMetrics( qwidget()->font() );
#endif



	const_cast<uiObjectBody*>(this)->fnt_hgt = fm->lineSpacing() + 2;
	const_cast<uiObjectBody*>(this)->fnt_wdt = fm->width(QChar('x'));
	const_cast<uiObjectBody*>(this)->fnt_maxwdt = fm->maxWidth();
    }

    if ( fnt_hgt<0 || fnt_hgt>100 )
    { 
	pErrMsg("Font heigt no good. Taking 25."); 
	const_cast<uiObjectBody*>(this)->fnt_hgt = 25;
    }
    if ( fnt_wdt<0 || fnt_wdt>100 )
    { 
	pErrMsg("Font width no good. Taking 10."); 
	const_cast<uiObjectBody*>(this)->fnt_wdt = 10;
    }
    if ( fnt_maxwdt<0 || fnt_maxwdt>100 )
    { 
	pErrMsg("Font maxwidth no good. Taking 15."); 
	const_cast<uiObjectBody*>(this)->fnt_maxwdt = 15;
    }
}
