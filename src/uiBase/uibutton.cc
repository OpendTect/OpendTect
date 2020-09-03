/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/

#include "uitoolbutton.h"
#include "i_qbutton.h"

#include "uiaction.h"
#include "uibuttongroup.h"
#include "uiicon.h"
#include "uimain.h"
#include "uimenu.h"
#include "uiobjbody.h"
#include "uipixmap.h"
#include "uitoolbar.h"
#include "uistrings.h"

#include "objdisposer.h"
#include "odiconfile.h"

#include "q_uiimpl.h"

#include <QApplication>
#include <QCheckBox>
#include <QEvent>
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
    : uiObjectBody(p,toString(txt))
    , messenger_(qbut,*this)
    , qbut_(qbut)
    , uibut_(uibut)
{
    qbut_.setText( toQString(txt) );
    setHSzPol( uiObject::SmallVar );
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
};


class IconUpdateEvent : public QEvent
{
public:
IconUpdateEvent( const char* iconnm )
    : QEvent(QEvent::User)
    , iconnm_(iconnm)
{}

BufferString iconnm_;
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


virtual const QWidget* qwidget_() const
{ return this; }


void keyPressEvent( QKeyEvent* qke )
{
    if ( qke && qke->key() == Qt::Key_Space )
	return;

    return ButT::keyPressEvent( qke );
}


void resizeEvent( QResizeEvent* ev )
{
    uiParent* hpar = uibut_.parent();
    mDynamicCastGet(uiToolBar*,tb,hpar)
    if ( !tb )
	uibut_.updateIconSize();

    QAbstractButton::resizeEvent( ev );
}


void customEvent( QEvent* ev )
{
    mDynamicCastGet(IconUpdateEvent*,iue,ev)
    if ( !iue ) return;

    handle_.setIcon( iue->iconnm_ );
}


virtual void setFont( const QFont& )
{
    if ( !uifont() ) { pErrMsg("no uifont!"); return; }
    QAbstractButton::setFont( uifont()->qFont() );
}


virtual void fontChange( const QFont& oldFont )
{ uiBody::fontchanged(); }


virtual void closeEvent( QCloseEvent* e )
{
    if ( uiCloseOK() )
	QAbstractButton::closeEvent(e);
}


protected:
virtual uiObject& uiObjHandle()
{ return handle_; }

    uiButton&		handle_;

};

#define mDefButtonBodyClass(nm,reactsto,constr_code,extr) \
class ui##nm##Body : public uiButtonTemplBody<Q##nm> \
{ \
public: \
 \
ui##nm##Body( uiButton& uibut, uiParent* parnt, const uiString& txt ) \
    : uiButtonTemplBody<Q##nm>(uibut,parnt,txt) \
{ \
    setText( toQString(txt) ); \
    constr_code; \
} \
 \
ui##nm##Body( uiButton& uibut, const uiPixmap& pm, \
	      uiParent* parnt, const uiString& txt ) \
    : uiButtonTemplBody<Q##nm>(uibut,parnt,txt) \
{ \
    setText( toQString(txt) ); \
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
extr; \
 \
}


// uiButton

mDefButtonBodyClass(PushButton,clicked,,);
mDefButtonBodyClass(RadioButton,clicked,,);
mDefButtonBodyClass(CheckBox,toggled,,void nextCheckState());
mDefButtonBodyClass(ToolButton,clicked,setFocusPolicy( Qt::ClickFocus ),);


void uiCheckBoxBody::nextCheckState()
{
    Qt::CheckState state = checkState();
    if ( state==Qt::Unchecked )
	setCheckState( Qt::Checked );
    else
	setCheckState( Qt::Unchecked );
}


#define muiButBody() dynamic_cast<uiButtonBody&>( *body() )

uiButton::uiButton( uiParent* parnt, const uiString& nm, const CallBack* cb,
		    uiObjectBody& b  )
    : uiObject(parnt,toString(nm),b)
    , activated(this)
    , iconscale_(0.75)
    , text_(nm)
{
    if ( cb ) activated.notify(*cb);

    mDynamicCastGet(uiButtonGroup*,butgrp,parnt)
    if ( butgrp )
	butgrp->addButton( this );
}


void uiButton::setIcon( const char* iconnm )
{
    if ( !isMainThreadCurrent() )
    {
	QApplication::postEvent( qButton(), new IconUpdateEvent(iconnm) );
	return;
    }

    uiIcon icon( iconnm );
    qButton()->setIcon( icon.qicon() );
    updateIconSize();
}


void uiButton::setPM( const uiPixmap& pm )
{
    if ( !isMainThreadCurrent() )
	return;

    qButton()->setIcon( *pm.qpixmap() );
    updateIconSize();
}


void uiButton::setIconScale( float val )
{
    if ( val<=0.0f || val>1.0f )
	val = 1.0f;

    iconscale_ = val;
    updateIconSize();
}


void uiButton::setText( const uiString& txt )
{
    text_ = txt;
    qButton()->setText( toQString(text_) );
}


void uiButton::translateText()
{
    uiObject::translateText();
    qButton()->setText( toQString(text_) );
}


static uiButton* crStd( uiParent* p, OD::StdActionType typ,
	const CallBack& cb, bool immediate, const uiString* buttxt,
	bool pbics=false )
{
    uiString txt = uiString::empty();
    uiString tt = uiString::empty();
    const char* icid = 0;

#   define mGetDefs(typ) \
    case OD::typ: { \
	if ( !buttxt ) \
	    txt = uiStrings::s##typ(); \
	else if ( !buttxt->isEmpty() )\
	{ \
	    txt = *buttxt; \
	    tt = uiStrings::phrThreeDots( uiStrings::s##typ(), immediate ); \
	} \
	icid = OD::IconFile::getIdentifier( OD::typ ); \
    break; }

    switch( typ )
    {
	mGetDefs(Apply)
	mGetDefs(Cancel)
	mGetDefs(Create)
	mGetDefs(Define)
	mGetDefs(Delete)
	mGetDefs(Edit)
	mGetDefs(Examine)
	mGetDefs(Export)
	mGetDefs(Help)
	mGetDefs(Import)
	mGetDefs(Ok)
	mGetDefs(Open)
	mGetDefs(Options)
	mGetDefs(Properties)
	mGetDefs(Reload)
	mGetDefs(Remove)
	mGetDefs(Rename)
	mGetDefs(Save)
	mGetDefs(SaveAs)
	mGetDefs(Select)
	mGetDefs(Settings)
	mGetDefs(Unload)
	mGetDefs(Video)
	default:
	break;
    }

    uiButton* ret = 0;
    if ( txt.isEmpty() )
	ret = new uiToolButton( p, icid, tt, cb );
    else
    {
	ret = new uiPushButton( p, txt, cb, immediate );
	if ( pbics )
	    ret->setIcon( icid );
    }

    return ret;
}

uiButton* uiButton::getStd( uiParent* p, OD::StdActionType typ,
	const CallBack& cb, bool immediate )
{
    return crStd( p, typ, cb, immediate, 0, true );
}

uiButton* uiButton::getStd( uiParent* p, OD::StdActionType typ,
	const CallBack& cb, bool immediate, const uiString& buttxt )
{
    return crStd( p, typ, cb, immediate, &buttxt, true );
}


const QAbstractButton* uiButton::qButton() const
{
    return dynamic_cast<const QAbstractButton*>( body() );
}


QAbstractButton* uiButton::qButton()
{
    return dynamic_cast<QAbstractButton*>( body() );
}


uiMenu* uiButton::addMenu()
{
    uiMenu* mnu = new uiMenu;
    QAbstractButton* qbut = qButton();
    mDynamicCastGet( QPushButton*, qpushbut, qbut );
    mDynamicCastGet( QToolButton*, qtoolbut, qbut );
    if ( qpushbut )
	qpushbut->setMenu( mnu->getQMenu() );
    else if ( qtoolbut )
	((uiToolButton*)this)->setMenu( mnu, uiToolButton::MenuButtonPopup );
    else
	{ pErrMsg("Can only set menu on push or tool button"); }
    return mnu;
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
			    const uiPixmap& pm, bool ia )
    : uiButton( parnt, nm, 0, mkbody(parnt,nm) )
    , immediate_(ia)
{
    updateText();
    setPixmap( pm );
}


uiPushButton::uiPushButton( uiParent* parnt, const uiString& nm,
			    const uiPixmap& pm, const CallBack& cb, bool ia )
    : uiButton( parnt, nm, &cb, mkbody(parnt,nm) )
    , immediate_(ia)
{
    updateText();
    setPixmap( pm );
}


uiPushButton* uiPushButton::getStd( uiParent* p, OD::StdActionType typ,
				    const CallBack& cb, bool immediate )
{
    uiButton* but = uiButton::getStd( p, typ, cb, immediate );
    if ( !but )
	return 0;

    mDynamicCastGet(uiPushButton*,pb,but)
    if ( !pb )
	{ pFreeFnErrMsg("uiButton::getStd delivered TB"); return 0; }

    return pb;
}


uiPushButtonBody& uiPushButton::mkbody( uiParent* parnt, const uiString& txt )
{
    pbbody_ = new uiPushButtonBody( *this, parnt, txt );
    return *pbbody_;
}


void uiPushButton::setFlat( bool yn )
{
    QAbstractButton* qbut = qButton();
    mDynamicCastGet(QPushButton*,qpushbut,qbut)
    if ( !qpushbut ) return;

    qpushbut->setFlat( yn );
}


bool uiPushButton::isFlat() const
{
    const QAbstractButton* qbut = qButton();
    mDynamicCastGet(const QPushButton*,qpushbut,qbut)
    return qpushbut ? qpushbut->isFlat() : false;
}


void uiPushButton::updateIconSize()
{
    int butwidth = qButton()->width();
    const int butheight = qButton()->height();
    if ( !text_.isEmpty() )
	butwidth = butheight;

    qButton()->setIconSize( QSize(mNINT32(butwidth*iconscale_),
				  mNINT32(butheight*iconscale_)) );
}


void uiPushButton::translateText()
{
    updateText();
}


void uiPushButton::updateText()
{
    mGetQStr( newtext, text_ );
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
    if ( check != isChecked() )
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


bool uiCheckBox::isChecked() const
{
    return cbbody_->isChecked();
}


void uiCheckBox::setChecked( bool yn )
{
    mBlockCmdRec;
    if ( yn != isChecked() )
	cbbody_->setChecked( yn );
}


void uiCheckBox::setTriState( bool yn )
{
    cbbody_->setTristate( yn );
}


void uiCheckBox::setCheckState( OD::CheckState cs )
{
    Qt::CheckState qcs = cs==OD::Unchecked ? Qt::Unchecked :
	(cs==OD::Checked ? Qt::Checked : Qt::PartiallyChecked);
    cbbody_->setCheckState( qcs );
}


OD::CheckState uiCheckBox::getCheckState() const
{
    Qt::CheckState qcs = cbbody_->checkState();
    return qcs==Qt::Unchecked ? OD::Unchecked :
	(qcs==Qt::Checked ? OD::Checked : OD::PartiallyChecked);
}


void uiCheckBox::click()
{
    setChecked( !isChecked() );
    activated.trigger();
}


uiButton* uiToolButtonSetup::getButton( uiParent* p, bool forcetb ) const
{
    const BufferString nm( toString(name_) );
    const bool istoolbut = nm == toString( tooltip_ );
    if ( forcetb || istoggle_ || istoolbut )
	return new uiToolButton( p, *this );

    return getPushButton( p, true );
}


uiToolButton* uiToolButtonSetup::getToolButton( uiParent* p ) const
{
    return new uiToolButton( p, *this );
}


uiPushButton* uiToolButtonSetup::getPushButton( uiParent* p, bool wic ) const
{
    uiPushButton* ret = 0;
    if ( wic )
	ret = new uiPushButton( p, name_, uiPixmap(icid_), cb_, isimmediate_ );
    else
	ret = new uiPushButton( p, name_, cb_, isimmediate_ );
    ret->setToolTip( tooltip_ );
    return ret;
}


// For some reason it is necessary to set the preferred width. Otherwise the
// button will reserve +- 3 times it's own width, which looks bad

#define mSetDefPrefSzs() \
    mDynamicCastGet(uiToolBar*,tb,parnt) \
    if ( !tb ) setPrefWidth( prefVNrPics() );

#define mInitTBList \
    id_(-1), uimenu_(0)

uiToolButton::uiToolButton( uiParent* parnt, const uiToolButtonSetup& su )
    : uiButton( parnt, su.name_, &su.cb_, mkbody(parnt,su.icid_,
		su.name_) )
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
		mkbody(parnt,fnm,tt) )
    , mInitTBList
{
    mSetDefPrefSzs();
    setToolTip( tt );
}


uiToolButton::uiToolButton( uiParent* parnt, uiToolButton::ArrowType at,
			    const uiString& tt, const CallBack& cb )
    : uiButton( parnt, tt, &cb,
		mkbody(parnt,"empty",tt) )
    , mInitTBList
{
    mSetDefPrefSzs();
    setArrowType( at );
    setToolTip( tt );
}


uiToolButton::~uiToolButton()
{
    delete uimenu_;
}


uiToolButtonBody& uiToolButton::mkbody( uiParent* parnt, const char* iconnm,
					const uiString& txt)
{
    tbbody_ = new uiToolButtonBody(*this,parnt,txt);
    uiIcon icon( iconnm );
    tbbody_->setIcon( icon.qicon() );
    const int sz = toolButtonSize();
    tbbody_->setIconSize( QSize(sz,sz) );
    return *tbbody_;
}


uiToolButton* uiToolButton::getStd( uiParent* p, OD::StdActionType typ,
				    const CallBack& cb, const uiString& tt )
{
    uiButton* but = uiButton::getStd( p, typ, cb, true,
					uiString::empty() );
    if ( !but )
	return 0;

    mDynamicCastGet(uiToolButton*,tb,but)
    if ( !tb )
	{ pFreeFnErrMsg("uiButton::getStd delivered PB"); return 0; }

    tb->setToolTip( tt );
    return tb;
}


bool uiToolButton::isOn() const { return tbbody_->isChecked(); }

void uiToolButton::setOn( bool yn )
{
    mBlockCmdRec;
    if ( yn != isOn() )
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


void uiToolButton::setNoMenu()
{
    setMenu( 0, DelayedPopup );
}


void uiToolButton::setMenu( uiMenu* mnu, PopupMode mode )
{
    const bool hasmenu = mnu;
    tbbody_->setMenu( hasmenu ? mnu->getQMenu() : 0 );

    mDynamicCastGet(uiToolBar*,tb,parent())
    if ( !tb )
    {
	if ( finalised() )
	{
	    QSize size = tbbody_->size();
	    const int wdth = size.height();
		// hasmenu ? mCast(int,1.5*size.height()) : size.height();
	    size.setWidth( wdth );
	    tbbody_->resize( size );
	}
	else
	{
	    const int wdth = prefVNrPics();
		// hasmenu ? mCast(int,1.5*prefVNrPics()) : prefVNrPics();
	    tbbody_->setPrefWidth( wdth );
	}
    }

    if ( !hasmenu )
	mode = DelayedPopup;
    tbbody_->setPopupMode( (QToolButton::ToolButtonPopupMode)mode );

    if ( uimenu_ )
	OBJDISP()->go( uimenu_ );
    uimenu_ = mnu;
}
