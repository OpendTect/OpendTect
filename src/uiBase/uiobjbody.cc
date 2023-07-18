/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiobjbody.h"

#include "uipixmap.h"
#include "color.h"
#include "timer.h"

#include "i_layout.h"
#include "i_layoutitem.h"

#include "q_uiimpl.h"

#include <QPixmap>

mUseQtnamespace

#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    #define mGetTextWidth(qfm,textstring) qfm.horizontalAdvance( textstring )
#else
    #define mGetTextWidth(qfm,textstring) qfm.width( textstring )
#endif


uiBody::uiBody()
{}


uiBody::~uiBody()
{}


uiObjectBody::uiObjectBody( uiParent* parnt, const char* nm )
    : uiBody()
    , NamedCallBacker(nm)
    , parent_(parnt ? dCast(uiParentBody*,parnt->body()) : nullptr)
{
#ifdef USE_DISPLAY_TIMER
    displaytimer_ = new Timer( "Display timer" );
    mAttachCB( displaytimer_->tick, uiObjectBody::doDisplay );
#endif
}


uiObjectBody::~uiObjectBody()
{
    sendDelNotif();
    detachAllNotifiers();
    delete layoutitem_;
}


void uiObjectBody::display( bool yn, bool shrink, bool maximized )
{
    display_ = yn;
    display_maximized_ = maximized;

    if ( !display_ && shrink )
    {
	pref_width_  = 0;
	pref_height_ = 0;
	is_hidden_ = true;
	qwidget()->hide();
    }
    else
    {
#ifdef USE_DISPLAY_TIMER
	if ( displaytimer_->isActive() )
	    displaytimer_->stop();

	displaytimer_->start( 0, true );
#else
	doDisplay( nullptr );
#endif
    }
}


void uiObjectBody::doDisplay( CallBacker* )
{
    if ( display_ )
    {
	is_hidden_ = false;
	if ( display_maximized_ )
	    qwidget()->showMaximized();
	else
	    qwidget()->show();
    }
    else
    {
	if ( !is_hidden_ )
	{
	    is_hidden_ = true;
	    qwidget()->hide();
	}
    }
}


void uiObjectBody::uisetFocus()
{
    qwidget()->setFocus();
}


bool uiObjectBody::uihasFocus() const
{
    return qwidget() ? qwidget()->hasFocus() : false;
}


void uiObjectBody::uisetSensitive( bool yn )
{
    qwidget()->setEnabled( yn );
}


bool uiObjectBody::uisensitive() const
{
    return qwidget() ? qwidget()->isEnabled() : false;
}


bool uiObjectBody::uivisible() const
{
    return qwidget() ? qwidget()->isVisible() : false;
}


void uiObjectBody::reDraw( bool deep )
{
    qwidget()->update();
}


i_LayoutItem* uiObjectBody::mkLayoutItem( i_LayoutMngr& mngr )
{
    if ( layoutitem_ )
	{ pErrMsg("Already have layout itm"); return layoutitem_ ; }

    layoutitem_ = mkLayoutItem_( mngr );
    if ( layoutitem_ )
	mAttachCB( layoutitem_->objectToBeDeleted(), uiObjectBody::itemDelCB );
    return layoutitem_;
}


void uiObjectBody::itemDelCB( CallBacker* )
{
    layoutitem_ = nullptr;
}


void uiObjectBody::finalize()
{
    if ( finalized_ )
	return;

    uiObjHandle().preFinalize().trigger( uiObjHandle() );
    finalize_();
    finalized_ = true;
    uiObjHandle().postFinalize().trigger( uiObjHandle() );
    if ( !display_ )
	display( display_ );
}


void uiObjectBody::fontchanged()
{
    fnt_hgt_ = 0;
    fnt_wdt_ = 0;
    fnt_maxwdt_ = 0;
    pref_width_hint_ = 0;
    pref_height_hint_ = 0;
}


int uiObjectBody::fontHeight() const
{
    gtFntWdtHgt();
    return fnt_hgt_;
}


int uiObjectBody::fontWidth( bool max ) const
{
    gtFntWdtHgt();
    return max ? fnt_maxwdt_ : fnt_wdt_;
}


void uiObjectBody::setHSzPol( uiObject::SzPolicy pol )
{
    hszpol_ = pol;
    if ( pol>=uiObject::SmallMax && pol<=uiObject::WideMax )
    {
	const int vs = stretch( false, true );
	setStretch( 2, vs );
    }
}


void uiObjectBody::setVSzPol( uiObject::SzPolicy pol )
{
    vszpol_ = pol;
    if ( pol>=uiObject::SmallMax && pol<=uiObject::WideMax )
    {
	const int hs = stretch( true, true );
	setStretch( hs, 2 );
    }
}


OD::Color uiObjectBody::uibackgroundColor() const
{
    const QBrush& qbr = qwidget()->palette().brush(
	    qwidget()->backgroundRole() );
    return OD::Color( qbr.color().rgb() );
}


void uiObjectBody::uisetBackgroundColor( const OD::Color& col )
{
    QPalette qpal( qwidget()->palette() );
    qpal.setColor( QPalette::Base,
		   QColor(col.r(),col.g(),col.b(),255-col.t()) );
    qwidget()->setPalette( qpal );
}


void uiObjectBody::uisetBackgroundPixmap( const uiPixmap& pm )
{
    QPalette qpal;
    qpal.setBrush( qwidget()->backgroundRole(), QBrush(*pm.qpixmap()) );
    qwidget()->setPalette( qpal );
}


void uiObjectBody::uisetTextColor( const OD::Color& col )
{
    QPalette qpal( qwidget()->palette() );
    qpal.setColor( QPalette::WindowText,
		   QColor(col.r(),col.g(),col.b(),255-col.t()) );
    qwidget()->setPalette( qpal );
}


void uiObjectBody::getSzHint()
{
    if ( pref_width_hint_ && pref_height_hint_ ) return;
    if ( pref_width_hint_ || pref_height_hint_ )
	{ pErrMsg("Only 1 defined size.."); }

    uiSize sh = layoutitem_->prefSize();

    pref_width_hint_ = sh.hNrPics();
    pref_height_hint_ = sh.vNrPics();
}


int uiObjectBody::prefHNrPics() const
{
    if ( pref_width_ > 0 )
	return pref_width_;

    if ( !layoutitem_ )
	return 0;

    if ( is_hidden_ )
	return pref_width_;

    if ( pref_width_set_ >= 0 )
    {
	const_cast<uiObjectBody*>(this)->pref_width_ = pref_width_set_;
    }
    else if ( pref_char_width_ >= 0 )
    {
	const int fw = fontWidth();
	if ( !fw )
	    { pErrMsg("Font has 0 width."); return 0; }

	const_cast<uiObjectBody*>(this)->pref_width_ =
					 mNINT32( pref_char_width_ * fw );
    }
    else
    {
	const_cast<uiObjectBody*>(this)->getSzHint();
	const int baseFldSz = uiObject::baseFldSize();

	int pwc=0;
	bool var=false;
	switch( szPol(true) )
	{
	    case uiObject::Small:    pwc=baseFldSz;	break;
	    case uiObject::Medium:   pwc=2*baseFldSz+1; break;
	    case uiObject::Wide:     pwc=4*baseFldSz+3; break;

	    case uiObject::SmallMax:
	    case uiObject::SmallVar: pwc=baseFldSz;	var=true; break;

	    case uiObject::MedMax:
	    case uiObject::MedVar:   pwc=2*baseFldSz+1; var=true; break;

	    case uiObject::WideMax:
	    case uiObject::WideVar:  pwc=4*baseFldSz+3; var=true; break;
	    default:
		break;
	}

	if ( !pwc )
	    const_cast<uiObjectBody*>(this)->pref_width_ = pref_width_hint_;
	else
	{
	    const int fw = fontWidth();
	    if ( !fw )
		{ pErrMsg("Font has 0 width."); }

	    const int pw = var ? (mMAX(pref_width_hint_,fw*pwc)) : fw*pwc;
	    const_cast<uiObjectBody*>(this)->pref_width_ = pw;
	}
    }

    return pref_width_;
}


void uiObjectBody::setPrefWidth( int w )
{
    if ( itemInited() )
    {
	if ( pref_width_set_ > 0 && pref_width_set_ != w )
	    { pErrMsg("Not allowed when finalized."); }
	return;
    }

    pref_char_width_ = -1;
    pref_width_set_ = w;
    pref_width_  = 0;
    pref_height_ = 0;
}


float uiObjectBody::prefWidthInCharSet() const
{
    return pref_char_width_;
}


void uiObjectBody::setPrefWidthInChar( float w )
{
    if ( itemInited() )
    {
	if ( pref_char_width_ != w )
	    { pErrMsg("Not allowed when finalized."); }
	return;
    }

    pref_width_set_ = -1;
    pref_char_width_ = w;
    pref_width_  = 0;
    pref_height_ = 0;
}


void uiObjectBody::setMinimumWidth( int w )
{ qwidget()->setMinimumWidth( w ); }

void uiObjectBody::setMinimumHeight( int h )
{ qwidget()->setMinimumHeight( h ); }

void uiObjectBody::setMaximumWidth( int w )
{ qwidget()->setMaximumWidth( w ); }

void uiObjectBody::setMaximumHeight( int h )
{ qwidget()->setMaximumHeight( h ); }


int uiObjectBody::prefVNrPics() const
{
    if ( pref_height_ > 0 )
	return pref_height_;

    if ( !layoutitem_ )
	return 0;

    if ( is_hidden_ )
	return pref_height_;

    if ( pref_height_set_ >= 0 )
    {
	const_cast<uiObjectBody*>(this)->pref_height_ = pref_height_set_;
    }
    else if ( pref_char_height_ >= 0 )
    {
	const int fh = fontHeight();
	if ( !fh )
	    { pErrMsg("Font has 0 height."); return 0; }

	const_cast<uiObjectBody*>(this)->pref_height_ =
					mNINT32( pref_char_height_ * fh + 5 );
    }
    else
    {
	const_cast<uiObjectBody*>(this)->getSzHint();

	if ( nrTxtLines() < 0 && szPol(false) == uiObject::Undef  )
	    const_cast<uiObjectBody*>(this)->pref_height_= pref_height_hint_;
	else
	{
	    float lines = 1.51;
	    if ( nrTxtLines() == 0 )
		lines = 7;
	    else if ( nrTxtLines() > 1 )
		lines = nrTxtLines();

	    const int baseFldSz = 1;
	    bool var = false;
	    switch( szPol(false) )
	    {
		case uiObject::Small:	 lines=baseFldSz;     break;
		case uiObject::Medium:	 lines=2*baseFldSz+1; break;
		case uiObject::Wide:	 lines=4*baseFldSz+3; break;

		case uiObject::SmallMax:
		case uiObject::SmallVar:
			lines=baseFldSz; var=true; break;

		case uiObject::MedMax:
		case uiObject::MedVar:
			lines=2*baseFldSz+1; var=true; break;

		case uiObject::WideMax:
		case uiObject::WideVar:
			lines=4*baseFldSz+3; var=true; break;
		default:
		    break;
	    }

	    const int fh = fontHeight();
	    if ( !fh )
		{ pErrMsg("Font has 0 height."); return 0; }

	    const int phc = mNINT32( lines * fh + 5 );
	    const int ph = var ? (mMAX(pref_height_hint_,phc)) : phc;
	    const_cast<uiObjectBody*>(this)->pref_height_ = ph;
	}
    }

    return pref_height_;
}


void uiObjectBody::setPrefHeight( int h )
{
    if ( itemInited() )
    {
	if ( pref_height_set_ != h )
	{
	    pErrMsg("Not allowed when finalized.");
	}
	return;
    }

    pref_char_height_ = -1;
    pref_height_set_ = h;
    pref_width_  = 0;
    pref_height_ = 0;
}


float uiObjectBody::prefHeightInCharSet() const
{
    return pref_char_height_;
}


void uiObjectBody::setPrefHeightInChar( float h )
{
    if ( itemInited() )
    {
	if ( pref_char_height_ != h )
	{
	    pErrMsg("Not allowed when finalized.");
	}
	return;
    }

    pref_height_set_ = -1;
    pref_char_height_ = h;
    pref_width_  = 0;
    pref_height_ = 0;
}


void uiObjectBody::setStretch( int hor, int ver )
{
    if ( itemInited() )
    {
	if ( hstretch_ != hor || vstretch_ != ver )
	    { pErrMsg("Not allowed when finalized."); }
	return;
    }

    hstretch_ = hor;
    vstretch_ = ver;
}


int uiObjectBody::stretch( bool hor, bool retundef ) const
{
    const int str = hor ? hstretch_ : vstretch_;
    if ( retundef )
	return str;

    return mIsUdf(str) ? 0 : str;
}


uiSize uiObjectBody::actualSize( bool include_border ) const
{
    return layoutitem_->actualSize( include_border );
}


void uiObjectBody::setToolTip( const uiString& txt )
{
    qwidget()->setToolTip( toQString(txt) );
}


void uiObjectBody::setCaption( const uiString& str )
{
    qwidget()->setWindowTitle( toQString(str) );
}


i_LayoutItem* uiObjectBody::mkLayoutItem_( i_LayoutMngr& mngr )
{
    return new i_uiLayoutItem( mngr , *this );
}


/*!
    attaches to parent if other=nullptr
*/
void uiObjectBody::attach ( constraintType tp, uiObject* other, int margin,
			    bool reciprocal )
{
//    parent_->attachChild( tp, this, other, margin );
    parent_->attachChild( tp, &uiObjHandle(), other, margin, reciprocal );
}


void uiObjectBody::attach( constraintType tp, uiParent* other, int margin,
			   bool reciprocal )
{
    attach( tp, other->mainObject(), margin, reciprocal );
}


const uiFont* uiObjectBody::uifont() const
{
    if ( !font_ )
    {
	QFont qf( qwidget()->font() );
	const_cast<uiObjectBody*>(this)->font_ = &FontList().getFromQfnt(&qf);
	const_cast<uiObjectBody*>(this)->qwidget()->setFont( font_->qFont() );
    }

    return font_;
}


void uiObjectBody::uisetFont( const uiFont& f )
{
    font_ = &f;
    qwidget()->setFont( font_->qFont() );
}


int uiObjectBody::fontWidthFor( const uiString& str ) const
{
    const QWidget* qw = qwidget();
    if ( !qw )
    {
	gtFntWdtHgt();
	QString qstr;
	str.fillQString( qstr );
	return qstr.size() * fnt_wdt_;
    }

    return mGetTextWidth(qw->fontMetrics(),toQString(str));
}


int uiObjectBody::fontWidthFor( const char* str ) const
{
    const QWidget* qw = qwidget();
    if ( !qw )
	{ gtFntWdtHgt(); return strlen(str) * fnt_wdt_; }

    return mGetTextWidth(qw->fontMetrics(),QString(str));
}


bool uiObjectBody::itemInited() const
{
    return layoutitem_ ? layoutitem_->inited() : false;
}


void uiObjectBody::gtFntWdtHgt() const
{
    if ( fnt_hgt_!=0 && fnt_wdt_!=0 && fnt_maxwdt_!=0 )
	return;

    uiObjectBody& self = *const_cast<uiObjectBody*>(this);

    QFont qft = QFont();
    const QFont& qfonttouse = font_ ? font_->qFont() : qft;
    QFontMetrics qfm( qfonttouse );
    self.fnt_hgt_ = qfm.lineSpacing();
    self.fnt_wdt_ = mGetTextWidth(qfm,QChar('x'));

    self.fnt_maxwdt_ = qfm.maxWidth();

    if ( fnt_hgt_<=0 || fnt_hgt_>100 )
    {
	pErrMsg( "Font height not good. Taking 15." );
	self.fnt_hgt_ = 15;
    }

    if ( fnt_wdt_<=0 || fnt_wdt_>100 )
    {
	pErrMsg( "Font width not good. Taking 10." );
	self.fnt_wdt_ = 10;
    }

    if ( fnt_maxwdt_<=0 || fnt_maxwdt_>100 )
    {
	for ( char idx=32; idx<127; idx++ )
	{
	    QChar ch( idx );
	    if ( ch.isPrint() )
	    {
		const int width = mGetTextWidth(qfm,ch);
		if ( width>self.fnt_maxwdt_ )
		    self.fnt_maxwdt_ = width;
	    }
	}

	if ( fnt_maxwdt_<=0 )
	{
	    pErrMsg( "Font maxwidth not good -> 15." );
	    self.fnt_maxwdt_ = 15;
	}
    }
}
