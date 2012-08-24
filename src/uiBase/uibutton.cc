/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uibutton.cc,v 1.79 2012-08-24 07:13:40 cvsnageswara Exp $";

#include "uitoolbutton.h"
#include "i_qbutton.h"

#include "uibuttongroup.h"
#include "uiicons.h"
#include "uimain.h"
#include "uimenu.h"
#include "uiobjbody.h"
#include "uitoolbar.h"
#include "pixmap.h"
#include "settings.h"


#include <QCheckBox>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QToolButton>


//! Wrapper around QButtons. 
/*!
    Extends each QButton class <T> with a i_ButMessenger, which connects
    itself to the signals transmitted from Qt buttons.  Each signal is
    relayed to the notifyHandler of a uiButton handle object.
*/
template< class T > class uiButtonTemplBody : public uiButtonBody,
    public uiObjectBody, public T { public:

			uiButtonTemplBody( uiButton& hndle, uiParent* p,
					   const char* txt )
			    : uiObjectBody( p, txt )
                            , T(p && p->pbody() ? p->pbody()->managewidg() : 0 )
                            , handle_( hndle )
			    , messenger_ ( *new i_ButMessenger( this, this) )
			    , idInGroup( 0 )		
			    { 
				this->setText(txt); 
				setHSzPol( uiObject::SmallVar );
			    }

			uiButtonTemplBody(uiButton& hndle, 
				     const ioPixmap& pm,
				     uiParent* parnt, const char* txt)
			    : uiObjectBody( parnt, txt )
			    , T( mQtclass(QIcon)(*pm.qpixmap()),txt, 
					parnt && parnt->pbody() ?
					parnt->pbody()->managewidg() : 0 )
                            , handle_( hndle )
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

    virtual mQtclass(QAbstractButton&)    qButton() = 0;
    inline const mQtclass(QAbstractButton&) qButton() const
                        { return ((uiButtonTemplBody*)this)->qButton(); }

    virtual int 	nrTxtLines() const		{ return 1; }

    const char*		text();

protected:

    i_ButMessenger&     messenger_;
    int                 idInGroup;

    void		doNotify()
			{
			    const int refnr = handle_.beginCmdRecEvent();
			    handle_.activated.trigger(handle_);
			    handle_.endCmdRecEvent( refnr );
			}
};

class uiPushButtonBody : public uiButtonTemplBody<mQtclass(QPushButton)>
{
public:
			uiPushButtonBody( uiButton& hndle, 
					  uiParent* parnt, const char* txt )
		     : uiButtonTemplBody<mQtclass(QPushButton)>(hndle,parnt,txt)
		     , iconfrac_(0.75)
		     {}

			uiPushButtonBody( uiButton& hndle, const ioPixmap& pm,
				          uiParent* parnt, const char* txt )
			    : uiButtonTemplBody<mQtclass(QPushButton)>
					(hndle,pm,parnt,txt)
			    , iconfrac_(0.75)
			    {}

    void		setIconFrac( float icf )
			{
			    if ( icf<=0.0 || icf>1.0 ) return;
#ifdef __win__
			    setIconSize( qbutsize_ );
#else
			    setIconSize( mQtclass(QSize)(mNINT32(width()*icf),
					       mNINT32(height()*icf)) );
#endif
			    iconfrac_ = icf;
			}

    virtual mQtclass(QAbstractButton&)    qButton()	{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) doNotify(); }

    void		resizeEvent( mQtclass(QResizeEvent*) ev )
			{
			    uiParent* hpar = handle_.parent();
			    mDynamicCastGet(uiToolBar*,tb,hpar)
			    if ( hpar && !tb )
			    {
			        if ( ev ) qbutsize_ = ev->size();
				setIconFrac( iconfrac_ );
			    }

			    mQtclass(QPushButton)::resizeEvent( ev );
			}

    float			iconfrac_;
    mQtclass(QSize)		qbutsize_;
};


class uiRadioButtonBody : public uiButtonTemplBody<mQtclass(QRadioButton)>
{                        
public:
			uiRadioButtonBody(uiButton& hndle, 
				     uiParent* parnt, const char* txt)
		    : uiButtonTemplBody<mQtclass(QRadioButton)>(hndle,parnt,txt)
		    {}

    virtual mQtclass(QAbstractButton&)    qButton()	{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) doNotify(); }
};


class uiCheckBoxBody: public uiButtonTemplBody<mQtclass(QCheckBox)>
{
public:

			uiCheckBoxBody(uiButton& hndle, 
				     uiParent* parnt, const char* txt)
		       : uiButtonTemplBody<mQtclass(QCheckBox)>(hndle,parnt,txt)
		       {}

    virtual mQtclass(QAbstractButton&)    qButton()	{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::toggled ) doNotify(); }
};


class uiToolButtonBody : public uiButtonTemplBody<mQtclass(QToolButton)>
{
public:
			uiToolButtonBody(uiButton& hndle, 
				     uiParent* parnt, const char* txt)
		     : uiButtonTemplBody<mQtclass(QToolButton)>(hndle,parnt,txt)
		      {
			  setFocusPolicy( mQtclass(Qt)::ClickFocus );
		      }


    virtual mQtclass(QAbstractButton&)    qButton()	{ return *this; }


protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) doNotify(); }
};


#define mqbut()         dynamic_cast<mQtclass(QAbstractButton*)>( body() )

uiButton::uiButton( uiParent* parnt, const char* nm, const CallBack* cb,
		    uiObjectBody& b  )
    : uiObject( parnt, nm, b )
    , activated( this )
{
    if ( cb ) activated.notify(*cb);

    mDynamicCastGet(uiButtonGroup*,butgrp,parnt)
    if ( butgrp ) butgrp->addButton( this );
}


void uiButton::setText( const char* txt )
{
    mqbut()->setText( mQtclass(QString)( txt ) );
}


const char* uiButton::text()
{
    static BufferString buttxt;
    buttxt = mQStringToConstChar( mqbut()->text() );
    return buttxt.buf();
}


mQtclass(QAbstractButton*) uiButton::qButton()
{
    return dynamic_cast<mQtclass(QAbstractButton*)>( body() );
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


void uiPushButton::setPixmap( const char* pmnm )
{
    setPixmap( ioPixmap(pmnm) );
}


void uiPushButton::setPixmap( const ioPixmap& pm )
{
    if ( !isMainThreadCurrent() )
	return;

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

void uiRadioButton::setChecked( bool check )
{
    mBlockCmdRec;
    body_->setChecked( check );
}

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
    mqbut()->setText( mQtclass(QString)( txt ) );
}


uiCheckBoxBody& uiCheckBox::mkbody( uiParent* parnt, const char* txt )
{ 
    body_= new uiCheckBoxBody(*this,parnt,txt);
    return *body_; 
}

bool uiCheckBox::isChecked () const		{ return body_->isChecked(); }


void uiCheckBox::setChecked ( bool check )
{
    mBlockCmdRec;
    body_->setChecked( check );
}


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
	body_->setIconSize( mQtclass(QSize)(iconSize(),iconSize()) ); \
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
			    const char* tt, const CallBack& cb )
    : uiButton( parnt, tt, &cb,
	        mkbody(parnt,ioPixmap(fnm),tt) )
    , mInitTBList
{
    mSetDefPrefSzs();
    setToolTip( tt );
}


uiToolButton::uiToolButton( uiParent* parnt, uiToolButton::ArrowType at,
			    const char* tt, const CallBack& cb )
    : uiButton( parnt, tt, &cb,
	        mkbody(parnt,ioPixmap(uiIcon::None()),tt) )
    , mInitTBList
{
    mSetDefPrefSzs();
    setArrowType( at );
    setToolTip( tt );
}


uiToolButton::~uiToolButton()
{
    delete qmenu_;
    delete uimenu_;
}


uiToolButtonBody& uiToolButton::mkbody( uiParent* parnt, const ioPixmap& pm,
					const char* txt)
{
    body_ = new uiToolButtonBody(*this,parnt,txt); 
    if ( pm.qpixmap() )
        body_->setIcon( *pm.qpixmap() );

    return *body_;
}


bool uiToolButton::isOn() const		{ return body_->isChecked(); }

void uiToolButton::setOn( bool yn )
{
    mBlockCmdRec;
    body_->setChecked( yn );
}


bool uiToolButton::isToggleButton() const     { return body_->isCheckable();}
void uiToolButton::setToggleButton( bool yn ) { body_->setCheckable( yn ); }


void uiToolButton::click()
{
    if ( isToggleButton() )
	setOn( !isOn() );
    activated.trigger();
}


void uiToolButton::setPixmap( const char* pmnm )
{
    setPixmap( ioPixmap(pmnm) );
}

void uiToolButton::setPixmap( const ioPixmap& pm )
{
    if ( !isMainThreadCurrent() )
	return;

    body_->setIcon( mQtclass(QIcon)(*pm.qpixmap()) );
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
    body_->setArrowType( (mQtclass(Qt)::ArrowType)(int)type );
#endif
}


void uiToolButton::setShortcut( const char* sc )
{
    body_->setShortcut( mQtclass(QString)(sc) );
}


void uiToolButton::setMenu( uiPopupMenu* mnu )
{
    delete qmenu_; delete uimenu_;
    uimenu_ = mnu;
    if ( !uimenu_ ) return;

    qmenu_ = new mQtclass(QMenu);
    for ( int idx=0; idx<mnu->nrItems(); idx++ )
    {
	mQtclass(QAction*) qact =
	    	 const_cast<mQtclass(QAction*)>( mnu->items()[idx]->qAction() );
	qmenu_->addAction( qact );
    }

    body_->setMenu( qmenu_ );
    body_->setPopupMode( QToolButton::MenuButtonPopup );
}
