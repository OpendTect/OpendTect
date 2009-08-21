/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uibutton.cc,v 1.64 2009-08-21 06:30:12 cvsnanne Exp $";

#include "uibutton.h"
#include "i_qbutton.h"

#include "uibuttongroup.h"
#include "uimenu.h"
#include "uiobjbody.h"
#include "uitoolbar.h"
#include "pixmap.h"
#include "settings.h"


#include <QApplication>
#include <QCheckBox>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QToolButton>

static const QEvent::Type sQEventActivate = (QEvent::Type) (QEvent::User + 0);

//! Wrapper around QButtons. 
/*!
    Extends each QButton class <T> with a i_ButMessenger, which connects
    itself to the signals transmitted from Qt buttons.  Each signal is
    relayed to the notifyHandler of a uiButton handle object.
*/
template< class T > class uiButtonTemplBody : public uiButtonBody,
    public uiObjectBody, public T { public:

			uiButtonTemplBody( uiButton& handle, uiParent* parnt,
					   const char* txt )
			    : uiObjectBody( parnt, txt )
                            , T( parnt && parnt->pbody() ?
				      parnt->pbody()->managewidg() : 0 )
                            , handle_( handle )
			    , messenger_ ( *new i_ButMessenger( this, this) )
			    , idInGroup( 0 )		
			    { 
				this->setText(txt); 
				setHSzPol( uiObject::SmallVar );
			    }

			uiButtonTemplBody(uiButton& handle, 
				     const ioPixmap& pm,
				     uiParent* parnt, const char* txt)
			    : uiObjectBody( parnt, txt )
			    , T( QIcon(*pm.qpixmap()),txt, 
					parnt && parnt->pbody() ?
					parnt->pbody()->managewidg() : 0 )
                            , handle_( handle )
			    , messenger_ ( *new i_ButMessenger( this, this) )
			    , idInGroup( 0 )		
			    { 
				this->setText(txt); 
				setHSzPol( uiObject::SmallVar );
			    }

#define mHANDLE_OBJ	uiButton
#include                "i_uiobjqtbody.h"

public:

    virtual		~uiButtonTemplBody()		{ delete &messenger_; }

    virtual QAbstractButton&    qButton() = 0;
    inline const QAbstractButton& qButton() const
                        { return ((uiButtonTemplBody*)this)->qButton(); }

    virtual int 	nrTxtLines() const		{ return 1; }

    const char*		text();

    void 		activate()
			{
			    QEvent* actevent = new QEvent( sQEventActivate );
			    QApplication::postEvent( &messenger_, actevent ); 
			}

protected:

    i_ButMessenger&     messenger_;
    int                 idInGroup;

    void		doNotify()
			{
			    const int refnr = handle_.beginCmdRecEvent();
			    handle_.activated.trigger(handle_);
			    handle_.endCmdRecEvent( refnr );
			}

    bool 		handleEvent( const QEvent* ev )
			{ 
			    if ( ev->type() != sQEventActivate ) return false;
			    handle_.click(); 
			    handle_.activatedone.trigger(handle_);
			    return true; 
			}

};

class uiPushButtonBody : public uiButtonTemplBody<QPushButton>
{
public:
			uiPushButtonBody( uiButton& handle, 
					  uiParent* parnt, const char* txt )
			    : uiButtonTemplBody<QPushButton>(handle,parnt,txt)
			    , iconfrac_(0.75)
			    {}

			uiPushButtonBody( uiButton& handle, const ioPixmap& pm,
				          uiParent* parnt, const char* txt )
			    : uiButtonTemplBody<QPushButton>
					(handle,pm,parnt,txt)
			    , iconfrac_(0.75)
			    {}

    void		setIconFrac( float icf )
			{
			    if ( icf<=0.0 || icf>1.0 ) return;
#ifdef __win__
			    setIconSize( qbutsize_ );
#else
			    setIconSize( QSize(mNINT(width()*icf),
					       mNINT(height()*icf)) );
#endif
			    iconfrac_ = icf;
			}

    virtual QAbstractButton&    qButton()		{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) doNotify(); }

    void		resizeEvent( QResizeEvent* ev )
			{
			    if ( ev ) qbutsize_ = ev->size();
			    setIconFrac( iconfrac_ );
			    QPushButton::resizeEvent( ev );
			}

    float		iconfrac_;
    QSize		qbutsize_;
};


class uiRadioButtonBody : public uiButtonTemplBody<QRadioButton>
{                        
public:
			uiRadioButtonBody(uiButton& handle, 
				     uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QRadioButton>(handle,parnt,txt)
			    {}

    virtual QAbstractButton&    qButton()		{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) doNotify(); }
};


class uiCheckBoxBody: public uiButtonTemplBody<QCheckBox>
{
public:

			uiCheckBoxBody(uiButton& handle, 
				     uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QCheckBox>(handle,parnt,txt)
			    {}

    virtual QAbstractButton&    qButton()		{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::toggled ) doNotify(); }
};


class uiToolButtonBody : public uiButtonTemplBody<QToolButton>
{
public:
			uiToolButtonBody(uiButton& handle, 
				     uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QToolButton>(handle,parnt,txt)
			    {
				setFocusPolicy( Qt::ClickFocus );
			    }


    virtual QAbstractButton&    qButton()		{ return *this; }


protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) doNotify(); }
};


#define mqbut()         dynamic_cast<QAbstractButton*>( body() )

uiButton::uiButton( uiParent* parnt, const char* nm, const CallBack* cb,
		    uiObjectBody& b  )
    : uiObject( parnt, nm, b )
    , activated( this )
    , activatedone( this )
{
    if ( cb ) activated.notify(*cb);

    mDynamicCastGet(uiButtonGroup*,butgrp,parnt)
    if ( butgrp ) butgrp->addButton( this );
}


void uiButton::setText( const char* txt )
{ 
    mqbut()->setText( QString( txt ) ); 
}


const char* uiButton::text()
{
    static BufferString buttxt;
    buttxt = mQStringToConstChar( mqbut()->text() );
    return buttxt.buf();
}


void uiButton::activate()
{
    dynamic_cast<uiButtonBody*>( body() )->activate();
}


QAbstractButton* uiButton::qButton()
{
    return dynamic_cast<QAbstractButton*>( body() );
}


uiPushButton::uiPushButton( uiParent* parnt, const char* nm, bool ia )
    : uiButton( parnt, nm, 0, mkbody(parnt,0,nm,ia) )
{}


uiPushButton::uiPushButton( uiParent* parnt, const char* nm, const CallBack& cb,
			    bool ia )
    : uiButton( parnt, nm, &cb, mkbody(parnt,0,nm,ia) )
{}


uiPushButton::uiPushButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm, bool ia )
    : uiButton( parnt, nm, 0, mkbody(parnt,&pm,nm,ia) )
{}


uiPushButton::uiPushButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm, const CallBack& cb, bool ia )
    : uiButton( parnt, nm, &cb, mkbody(parnt,&pm,nm,ia) )
{}


uiPushButton::~uiPushButton()
{
}


uiPushButtonBody& uiPushButton::mkbody( uiParent* parnt, const ioPixmap* pm,
					const char* txt, bool immact )
{
    BufferString buttxt( txt );
    if ( !immact && txt && *txt )
	buttxt += " ...";
    if ( pm )	body_ = new uiPushButtonBody(*this,*pm,parnt,buttxt.buf()); 
    else	body_ = new uiPushButtonBody(*this,parnt,buttxt.buf()); 

    return *body_; 
}


void uiPushButton::setDefault( bool yn )
{
    body_->setDefault( yn );
    setFocus();
}


void uiPushButton::setPixmap( const ioPixmap& pm )
{
    body_->setIconFrac( 0.7 );
    body_->setIcon( *pm.qpixmap() );
}


void uiPushButton::click()			{ activated.trigger(); }


uiRadioButton::uiRadioButton( uiParent* p, const char* nm )
    : uiButton(p,nm,0,mkbody(p,nm))
{}


uiRadioButton::uiRadioButton( uiParent* p, const char* nm, 
			      const CallBack& cb )
    : uiButton(p,nm,&cb,mkbody(p,nm))
{}


uiRadioButtonBody& uiRadioButton::mkbody( uiParent* parnt, const char* txt )
{ 
    body_= new uiRadioButtonBody(*this,parnt,txt);
    return *body_; 
}


bool uiRadioButton::isChecked() const		{ return body_->isChecked (); }

void uiRadioButton::setChecked( bool check )	{ body_->setChecked( check ); }

void uiRadioButton::click()			
{ 
    setChecked( !isChecked() );
    activated.trigger();
}


uiCheckBox::uiCheckBox( uiParent* p, const char* nm )
    : uiButton(p,nm,0,mkbody(p,nm))
{}


uiCheckBox::uiCheckBox( uiParent* p, const char* nm, const CallBack& cb )
    : uiButton(p,nm,&cb,mkbody(p,nm))
{}


void uiCheckBox::setText( const char* txt )
{ 
    mqbut()->setText( QString( txt ) ); 
}


uiCheckBoxBody& uiCheckBox::mkbody( uiParent* parnt, const char* txt )
{ 
    body_= new uiCheckBoxBody(*this,parnt,txt);
    return *body_; 
}

bool uiCheckBox::isChecked () const		{ return body_->isChecked(); }

void uiCheckBox::setChecked ( bool check )	{ body_->setChecked( check ); }

void uiCheckBox::click()			
{
    setChecked( !isChecked() );
    activated.trigger();
}


// For some reason it is necessary to set the preferred width. Otherwise the
// button will reserve +- 3 times it's own width, which looks bad

static int preftbsz = -1;
#define mSetDefPrefSzs() \
    if ( preftbsz < 0 ) \
	body_->setIconSize( QSize(iconSize(),iconSize()) ); \
    mDynamicCastGet(uiToolBar*,tb,parnt) \
    if ( !tb ) setPrefWidth( prefVNrPics() );

#define mInit \
    id_(-1), qmenu_(0), uimenu_(0)

uiToolButton::uiToolButton( uiParent* parnt, const char* nm )
    : uiButton( parnt, nm, 0, mkbody(parnt,0,nm) )
    , mInit
{
    mSetDefPrefSzs();
}


uiToolButton::uiToolButton( uiParent* parnt, const char* nm, const CallBack& cb)
    : uiButton( parnt, nm, &cb, mkbody(parnt,0,nm) )
    , mInit
{
    mSetDefPrefSzs();
}


uiToolButton::uiToolButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm )
    : uiButton( parnt, nm, 0, mkbody(parnt,&pm,nm) )
    , mInit
{
    mSetDefPrefSzs();
}


uiToolButton::uiToolButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm, const CallBack& cb )
    : uiButton( parnt, nm, &cb, mkbody(parnt,&pm,nm) )
    , mInit
{
    mSetDefPrefSzs();
}


uiToolButton::~uiToolButton()
{
    delete qmenu_;
    delete uimenu_;
}


uiToolButtonBody& uiToolButton::mkbody( uiParent* parnt, const ioPixmap* pm,
					const char* txt)
{
    body_ = new uiToolButtonBody(*this,parnt,txt); 
    if ( pm )
        body_->setIcon( *pm->qpixmap() );

    return *body_;
}


bool uiToolButton::isOn() const		{ return body_->isChecked(); }
void uiToolButton::setOn( bool yn )	{ body_->setChecked( yn ); }

bool uiToolButton::isToggleButton() const     { return body_->isCheckable();}
void uiToolButton::setToggleButton( bool yn ) { body_->setCheckable( yn ); }


void uiToolButton::click()
{
    if ( isToggleButton() )
	setOn( !isOn() );
    activated.trigger();
}


void uiToolButton::setPixmap( const ioPixmap& pm )
{
    body_->setIcon( QIcon(*pm.qpixmap()) );
}


void uiToolButton::setArrowType( ArrowType type )
{
#ifdef __win__
    switch ( type )
    {
	case UpArrow: setPixmap( ioPixmap("uparrow.png") ); break;
	case DownArrow: setPixmap( ioPixmap("downarrow.png") ); break;
	case LeftArrow: setPixmap( ioPixmap("leftarrow.png") ); break;
	case RightArrow: setPixmap( ioPixmap("rightarrow.png") ); break;
    }
#else
    body_->setArrowType( (Qt::ArrowType)(int)type );
#endif
}


void uiToolButton::setShortcut( const char* sc )
{
    body_->setShortcut( QString(sc) );
}


void uiToolButton::setMenu( uiPopupMenu* mnu )
{
    delete qmenu_; delete uimenu_;
    uimenu_ = mnu;
    if ( !uimenu_ ) return;

    qmenu_ = new QMenu;
    for ( int idx=0; idx<mnu->nrItems(); idx++ )
    {
	QAction* qact = const_cast<QAction*>( mnu->items()[idx]->qAction() );
	qmenu_->addAction( qact );
    }

    body_->setMenu( qmenu_ );
    body_->setPopupMode( QToolButton::MenuButtonPopup );
}
