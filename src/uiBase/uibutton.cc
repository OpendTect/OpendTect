/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uibutton.cc,v 1.20 2003-05-13 15:34:47 arend Exp $
________________________________________________________________________

-*/

#include <uibutton.h>
#include <i_qbutton.h>
#include <pixmap.h>

#include <uiobjbody.h>

#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qtoolbutton.h>

//! Wrapper around QButtons. 
/*!
    Extends each Qbutton class <T> with a i_ButMessenger, which connects itself 
    to the signals transmitted from Qt buttons.
    Each signal is relayed to the notifyHandler of a uiButton handle object.
*/
template< class T >
class uiButtonTemplBody : public uiButtonBody, public uiObjectBody, public T
{
public:

			uiButtonTemplBody( uiButton& handle, uiParent* parnt,
					   const char* txt )
			    : uiObjectBody( parnt, txt )
                            , T( parnt && parnt->pbody() ?
				  parnt->pbody()->managewidg() : 0 , txt )
                            , handle_( handle )
			    , messenger_ ( *new i_ButMessenger( this, this) )
			    , idInGroup( 0 )		
			    { 
				setText(txt); 
				setHSzPol( uiObject::smallvar );
			    }

			uiButtonTemplBody(uiButton& handle, 
				     const ioPixmap& pm,
				     uiParent* parnt, const char* txt)
			    : uiObjectBody( parnt, txt )
			    , T( QIconSet(*pm.Pixmap()),txt, 
				parnt && parnt->pbody() ?
				  parnt->pbody()->managewidg() : 0, txt )
                            , handle_( handle )
			    , messenger_ ( *new i_ButMessenger( this, this) )
			    , idInGroup( 0 )		
			    { 
				setText(txt); 
				setHSzPol( uiObject::smallvar );
			    }

#define mHANDLE_OBJ	uiButton
#include                "i_uiobjqtbody.h"

public:

    virtual		~uiButtonTemplBody()		{ delete &messenger_; }

    virtual QButton&    qButton() = 0;
    inline const QButton& qButton() const
                        { return ((uiButtonTemplBody*)this)->qButton(); }

    virtual int 	nrTxtLines() const		{ return 1; }

    const char*		text();

protected:

    i_ButMessenger&     messenger_;
    int                 idInGroup;

    void		Notifier()	{ handle_.activated.trigger(handle_); }

};

class uiPushButtonBody : public uiButtonTemplBody<QPushButton>
{
public:
			uiPushButtonBody(uiButton& handle, 
				     uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QPushButton>(handle,parnt,txt)
			    {}

			uiPushButtonBody( uiButton& handle, const ioPixmap& pm,
				          uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QPushButton>
					(handle,pm,parnt,txt)		{}

    virtual QButton&    qButton()		{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) Notifier(); }
};


class uiRadioButtonBody : public uiButtonTemplBody<QRadioButton>
{                        
public:
			uiRadioButtonBody(uiButton& handle, 
				     uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QRadioButton>(handle,parnt,txt)
			    {}

    virtual QButton&    qButton()		{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) Notifier(); }
};


class uiCheckBoxBody: public uiButtonTemplBody<QCheckBox>
{
public:

			uiCheckBoxBody(uiButton& handle, 
				     uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QCheckBox>(handle,parnt,txt)
			    {}

    virtual QButton&    qButton()		{ return *this; }

protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::stateChanged ) Notifier(); }
};


class uiToolButtonBody : public uiButtonTemplBody<QToolButton>
{
public:
			uiToolButtonBody(uiButton& handle, 
				     uiParent* parnt, const char* txt)
			    : uiButtonTemplBody<QToolButton>(handle,parnt,txt)
			    {}

    virtual QButton&    qButton()		{ return *this; }


protected:

    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == uiButtonBody::clicked ) Notifier(); }
};


#define mqbut()         dynamic_cast<QButton*>( body() )

uiButton::uiButton( uiParent* parnt, const char* nm, const CallBack* cb,
		    uiObjectBody& b  )
    : uiObject( parnt, nm, b )
    , activated( this )
    { if ( cb ) activated.notify(*cb); }

void uiButton::setText( const char* txt )
{ 
    mqbut()->setText( QString( txt ) ); 
}

const char* uiButton::text()
    { return mqbut()->text(); }


uiPushButton::uiPushButton( uiParent* parnt, const char* nm )
    : uiButton( parnt, nm, 0, mkbody(parnt,0,nm) )
{}
uiPushButton::uiPushButton( uiParent* parnt, const char* nm, const CallBack& cb)
    : uiButton( parnt, nm, &cb, mkbody(parnt,0,nm) )
{}


uiPushButton::uiPushButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm )
    : uiButton( parnt, nm, 0, mkbody(parnt,&pm,nm) )
{}
uiPushButton::uiPushButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm, const CallBack& cb )
    : uiButton( parnt, nm, &cb, mkbody(parnt,&pm,nm) )
{}

uiPushButtonBody& uiPushButton::mkbody( uiParent* parnt, const ioPixmap* pm,
					const char* txt)
{
    if( pm )	body_= new uiPushButtonBody(*this,*pm,parnt,txt); 
    else	body_= new uiPushButtonBody(*this,parnt,txt); 

    return *body_; 
}

void uiPushButton::setDefault( bool yn)
    { body_->setDefault( yn ); setFocus(); }



uiRadioButton::uiRadioButton(uiParent* parnt,const char* nm )
    : uiButton( parnt, nm, 0, mkbody(parnt,nm) )
{}

uiRadioButtonBody& uiRadioButton::mkbody(uiParent* parnt, const char* txt)
{ 
    body_= new uiRadioButtonBody(*this,parnt,txt);
    return *body_; 
}

bool uiRadioButton::isChecked() const 
    { return body_->isChecked (); }

void uiRadioButton::setChecked( bool check ) 
    { body_->setChecked( check );}



uiCheckBox::uiCheckBox( uiParent* parnt, const char* nm )
    : uiButton( parnt, nm, 0, mkbody(parnt,nm) )
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

bool uiCheckBox::isChecked () const 
    { return body_->isChecked(); }

void uiCheckBox::setChecked ( bool check ) 
    { body_->setChecked( check ); }


uiToolButton::uiToolButton( uiParent* parnt, const char* nm )
    : uiButton( parnt, nm, 0, mkbody(parnt,0,nm) )
{}
uiToolButton::uiToolButton( uiParent* parnt, const char* nm, const CallBack& cb)
    : uiButton( parnt, nm, &cb, mkbody(parnt,0,nm) )
{}

uiToolButton::uiToolButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm )
    : uiButton( parnt, nm, 0, mkbody(parnt,&pm,nm) )
{}
uiToolButton::uiToolButton( uiParent* parnt, const char* nm,
			    const ioPixmap& pm, const CallBack& cb )
    : uiButton( parnt, nm, &cb, mkbody(parnt,&pm,nm) )
{}

uiToolButtonBody& uiToolButton::mkbody( uiParent* parnt, const ioPixmap* pm,
					const char* txt)
{
    body_= new uiToolButtonBody(*this,parnt,txt); 

    if( pm )
        body_->setIconSet( QIconSet(*pm->Pixmap()) );

    return *body_;
}

bool uiToolButton::isOn()		{ return body_->isOn(); }
void uiToolButton::setOn( bool yn)	{ body_->setOn(yn); }

bool uiToolButton::isToggleButton()	     { return body_->isToggleButton(); }
void uiToolButton::setToggleButton( bool yn) { body_->setToggleButton(yn); }
