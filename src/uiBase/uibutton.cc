/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uitoolbutton.h"
#include "i_qbutton.h"

#include "uiaction.h"
#include "uibuttongroup.h"
#include "uiicons.h"
#include "uimain.h"
#include "uimenu.h"
#include "uiobjbody.h"
#include "uitoolbar.h"
#include "pixmap.h"
#include "settings.h"
#include "perthreadrepos.h"


#include <QCheckBox>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QToolButton>

mUseQtnamespace


class uiButtonBody : public uiObjectBody
		   , public uiButtonMessenger
{
public:

uiButtonBody( uiButton& uibut, uiParent* p, const uiString& txt,
	      QAbstractButton& qbut )
    : uiObjectBody(p,txt.getFullString())
    , messenger_(qbut,*this)
    , iconfrac_(0.75)
    , qbut_(qbut)
    , uibut_(uibut)
{
    qbut_.setText( txt.getQtString() );
    setHSzPol( uiObject::SmallVar );
}

void setIconFrac( float icf )
{
    if ( icf<=0.0f || icf>1.0f )
	icf = 1.0f;

#ifdef __win__
    qbut_.setIconSize( qbutsize_ );
#else
    qbut_.setIconSize( QSize(mNINT32(qbut_.width()*icf),
		       mNINT32(qbut_.height()*icf)) );
#endif

    iconfrac_ = icf;

}

void doNotify()
{
    const int refnr = uibut_.beginCmdRecEvent();
    uibut_.activated.trigger(uibut_);
    uibut_.endCmdRecEvent( refnr );
}

virtual int nrTxtLines() const
{
    return 1;
}

    uiButton&		uibut_;
    QAbstractButton&	qbut_;
    i_ButMessenger	messenger_;
    float		iconfrac_;
    QSize		qbutsize_;
};


//! Wrapper around QButtons.
/*!
    Extends each QButton class <ButT> with a i_ButMessenger, which connects
    itself to the signals transmitted from Qt buttons.	Each signal is
    relayed to the notifyHandler of a uiButton handle object.
*/

template<class ButT>
class uiButtonTemplBody : public ButT
			, public uiButtonBody
{
public:

uiButtonTemplBody( uiButton& uibut, uiParent* p, const uiString& txt )
    : uiButtonBody( uibut, p, txt, *this )
    , ButT( p && p->pbody() ? p->pbody()->managewidg() : 0 )
    , handle_(uibut)
{
}

#define mHANDLE_OBJ	uiButton
#define mQWIDGET_BODY	QAbstractButton
#include "i_uiobjqtbody.h"

void resizeEvent( QResizeEvent* ev )
{
    uiParent* hpar = uibut_.parent();
    mDynamicCastGet(uiToolBar*,tb,hpar)
    if ( hpar && !tb )
    {
	if ( ev )
	    qbutsize_ = ev->size();
	setIconFrac( iconfrac_ );
    }

    QAbstractButton::resizeEvent( ev );
}

void setPixmap( const ioPixmap& pm )
{
    qbut_.setIcon( QIcon(*pm.qpixmap()) );
}

};

#define mDefButtonBodyClass(nm,reactsto,constr_code) \
class ui##nm##Body : public uiButtonTemplBody<Q##nm> \
{ \
public: \
 \
ui##nm##Body( uiButton& uibut, uiParent* parnt, const uiString& txt ) \
    : uiButtonTemplBody<Q##nm>(uibut,parnt,txt) \
{ \
    setText( txt.getQtString() ); \
    constr_code; \
} \
 \
ui##nm##Body( uiButton& uibut, const ioPixmap& pm, \
	      uiParent* parnt, const uiString& txt ) \
    : uiButtonTemplBody<Q##nm>(uibut,parnt,txt) \
{ \
    setText( txt.getQtString() ); \
    setPixmap( pm ); \
    constr_code; \
} \
 \
protected: \
 \
virtual void notifyHandler( notifyTp tp ) \
{ \
    if ( tp == uiButtonMessenger::reactsto ) \
	doNotify(); \
} \
 \
}


// uiButton

mDefButtonBodyClass(PushButton,clicked,);
mDefButtonBodyClass(RadioButton,clicked,);
mDefButtonBodyClass(CheckBox,toggled,);
mDefButtonBodyClass(ToolButton,clicked,setFocusPolicy( Qt::ClickFocus ));

#define muiButBody() dynamic_cast<uiButtonBody&>( *body() )

uiButton::uiButton( uiParent* parnt, const uiString& nm, const CallBack* cb,
		    uiObjectBody& b  )
    : uiObject(parnt,nm.getFullString(),b)
    , activated(this)
    , text_(nm)
{
    if ( cb ) activated.notify(*cb);

    mDynamicCastGet(uiButtonGroup*,butgrp,parnt)
    if ( butgrp )
	butgrp->addButton( this );
}


void uiButton::setPixmap( const char* pmnm )
{
    setPM( ioPixmap(pmnm) );
}


void uiButton::setPM( const ioPixmap& pm )
{
    if ( !isMainThreadCurrent() )
	return;

    muiButBody().setIconFrac( 0.7 );
    qButton()->setIcon( *pm.qpixmap() );
}


void uiButton::setText( const uiString& txt )
{
    text_ = txt;
    qButton()->setText( text_.getQtString() );
}


void uiButton::translateText()
{
    uiObject::translateText();
    qButton()->setText( text_.getQtString() );
}


QAbstractButton* uiButton::qButton()
{
    return dynamic_cast<QAbstractButton*>( body() );
}


#define mQBut(typ) dynamic_cast<Q##typ&>( *body() )

// uiPushButton
uiPushButton::uiPushButton( uiParent* parnt, const uiString& nm, bool ia )
    : uiButton( parnt, nm, 0, mkbody(parnt,nm) )
    , immediate_(ia)
{
    updateText();
}


uiPushButton::uiPushButton( uiParent* parnt, const uiString& nm,
			    const CallBack& cb, bool ia )
    : uiButton( parnt, nm, &cb, mkbody(parnt,nm) )
    , immediate_(ia)
{
    updateText();
}


uiPushButton::uiPushButton( uiParent* parnt, const uiString& nm,
			    const ioPixmap& pm, bool ia )
    : uiButton( parnt, nm, 0, mkbody(parnt,nm) )
    , immediate_(ia)
{
    updateText();
    setPixmap( pm );
}


uiPushButton::uiPushButton( uiParent* parnt, const uiString& nm,
			    const ioPixmap& pm, const CallBack& cb, bool ia )
    : uiButton( parnt, nm, &cb, mkbody(parnt,nm) )
    , immediate_(ia)
{
    updateText();
    setPixmap( pm );
}


uiPushButtonBody& uiPushButton::mkbody( uiParent* parnt, const uiString& txt )
{
    pbbody_ = new uiPushButtonBody( *this, parnt, txt );
    return *pbbody_;
}


void uiPushButton::translateText()
{
    updateText();
}


void uiPushButton::updateText()
{
    QString newtext = text_.getQtString();
    if ( !newtext.isEmpty() && !immediate_ )
	newtext.append( " ..." );

    qButton()->setText( newtext );
}


void uiPushButton::setDefault( bool yn )
{
    mQBut(PushButton).setDefault( yn );
    setFocus();
}


void uiPushButton::click()
{
    activated.trigger();
}


// uiRadioButton
uiRadioButton::uiRadioButton( uiParent* p, const uiString& nm )
    : uiButton(p,nm,0,mkbody(p,nm))
{
}


uiRadioButton::uiRadioButton( uiParent* p, const uiString& nm,
			      const CallBack& cb )
    : uiButton(p,nm,&cb,mkbody(p,nm))
{
}


uiRadioButtonBody& uiRadioButton::mkbody( uiParent* parnt,
					  const uiString& txt )
{
    rbbody_ = new uiRadioButtonBody( *this, parnt, txt );
    return *rbbody_;
}


bool uiRadioButton::isChecked() const
{
    return rbbody_->isChecked ();
}

void uiRadioButton::setChecked( bool check )
{
    mBlockCmdRec;
    rbbody_->setChecked( check );
}


void uiRadioButton::click()
{
    setChecked( !isChecked() );
    activated.trigger();
}


// uiCheckBox
uiCheckBox::uiCheckBox( uiParent* p, const uiString& nm )
    : uiButton(p,nm,0,mkbody(p,nm))
{
}


uiCheckBox::uiCheckBox( uiParent* p, const uiString& nm, const CallBack& cb )
    : uiButton(p,nm,&cb,mkbody(p,nm))
{
}


uiCheckBoxBody& uiCheckBox::mkbody( uiParent* parnt, const uiString& txt )
{
    cbbody_ = new uiCheckBoxBody( *this, parnt, txt );
    return *cbbody_;
}


bool uiCheckBox::isChecked () const
{
    return cbbody_->isChecked();
}


void uiCheckBox::setChecked ( bool check )
{
    mBlockCmdRec;
    cbbody_->setChecked( check );
}


void uiCheckBox::click()
{
    setChecked( !isChecked() );
    activated.trigger();
}


uiButton* uiToolButtonSetup::getButton( uiParent* p, bool forcetb ) const
{
    if ( forcetb || istoggle_ || name_ == tooltip_ )
	return new uiToolButton( p, *this );

    uiPushButton* pb = new uiPushButton( p, name_, ioPixmap(filename_),
					 cb_, isimmediate_ );
    pb->setToolTip( tooltip_ );
    return pb;
}


// For some reason it is necessary to set the preferred width. Otherwise the
// button will reserve +- 3 times it's own width, which looks bad

static int preftbsz = -1;
#define mSetDefPrefSzs() \
    if ( preftbsz < 0 ) \
	tbbody_->setIconSize( QSize(iconSize(),iconSize()) ); \
    mDynamicCastGet(uiToolBar*,tb,parnt) \
    if ( !tb ) setPrefWidth( prefVNrPics() );

#define mInitTBList \
    id_(-1), qmenu_(0), uimenu_(0)

uiToolButton::uiToolButton( uiParent* parnt, const uiToolButtonSetup& su )
    : uiButton( parnt, su.name_, &su.cb_,
	        mkbody(parnt,ioPixmap(su.filename_),su.name_) )
    , mInitTBList
{
    setToolTip( su.tooltip_ );
    if ( su.istoggle_ )
    {
	setToggleButton( true );
	setOn( su.ison_ );
    }
    if ( su.arrowtype_ != NoArrow )
	setArrowType( su.arrowtype_ );
    if ( !su.shortcut_.isEmpty() )
	setShortcut( su.shortcut_ );

    mSetDefPrefSzs();
}


uiToolButton::uiToolButton( uiParent* parnt, const char* fnm,
			    const uiString& tt, const CallBack& cb )
    : uiButton( parnt, tt, &cb,
	        mkbody(parnt,ioPixmap(fnm),tt) )
    , mInitTBList
{
    mSetDefPrefSzs();
    setToolTip( tt.getFullString() );
}


uiToolButton::uiToolButton( uiParent* parnt, uiToolButton::ArrowType at,
			    const uiString& tt, const CallBack& cb )
    : uiButton( parnt, tt, &cb,
	        mkbody(parnt,ioPixmap(uiIcon::None()),tt) )
    , mInitTBList
{
    mSetDefPrefSzs();
    setArrowType( at );
    setToolTip( tt.getFullString() );
}


uiToolButton::~uiToolButton()
{
    delete qmenu_;
    delete uimenu_;
}


uiToolButtonBody& uiToolButton::mkbody( uiParent* parnt, const ioPixmap& pm,
					const uiString& txt)
{
    tbbody_ = new uiToolButtonBody(*this,parnt,txt);
    if ( pm.qpixmap() )
	tbbody_->setIcon( QIcon(*pm.qpixmap()) );

    return *tbbody_;
}


bool uiToolButton::isOn() const { return tbbody_->isChecked(); }

void uiToolButton::setOn( bool yn )
{
    mBlockCmdRec;
    tbbody_->setChecked( yn );
}


bool uiToolButton::isToggleButton() const     { return tbbody_->isCheckable();}
void uiToolButton::setToggleButton( bool yn ) { tbbody_->setCheckable( yn ); }


void uiToolButton::click()
{
    if ( isToggleButton() )
	setOn( !isOn() );
    activated.trigger();
}


void uiToolButton::setPM( const ioPixmap& pm )
{
    if ( !isMainThreadCurrent() )
	return;

    tbbody_->setIcon( QIcon(*pm.qpixmap()) );
}


void uiToolButton::setArrowType( ArrowType type )
{
#ifdef __win__
    switch ( type )
    {
	case UpArrow: setPixmap( "uparrow" ); break;
	case DownArrow: setPixmap( "downarrow" ); break;
	case LeftArrow: setPixmap( "leftarrow" ); break;
	case RightArrow: setPixmap( "rightarrow" ); break;
    }
#else
    tbbody_->setArrowType( (Qt::ArrowType)(int)type );
#endif
}


void uiToolButton::setShortcut( const char* sc )
{
    tbbody_->setShortcut( QString(sc) );
}


void uiToolButton::setMenu( uiMenu* mnu )
{
    delete qmenu_; delete uimenu_;
    uimenu_ = mnu;
    if ( !uimenu_ ) return;

    qmenu_ = new QMenu;
    for ( int idx=0; idx<mnu->nrActions(); idx++ )
    {
	QAction* qact =
		 const_cast<QAction*>( mnu->actions()[idx]->qaction() );
	qmenu_->addAction( qact );
    }

    tbbody_->setMenu( qmenu_ );
    tbbody_->setPopupMode( QToolButton::MenuButtonPopup );
}
