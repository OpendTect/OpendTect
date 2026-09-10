#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiobjbody.h"

/*!\brief Default (Template) implementation of uiObjectBody.
*/

template <class C, class T>
mClass(uiBase) uiObjBodyImpl : public uiObjectBody, public T
{
public:
			uiObjBodyImpl( C& hndle, uiParent* parnt,
				       const char* nm )
			    : uiObjectBody( parnt, nm )
			    , T( parnt && parnt->pbody() ?
				     parnt->pbody()->managewidg() : nullptr )
			    , handle_( hndle )
			{
			    this->setObjectName( nm );
			}
			~uiObjBodyImpl()
			{}
			mOD_DisableCopy(uiObjBodyImpl)

    const QWidget*	qwidget_() const override { return this; }
    virtual void	setFont( const QFont& )
			{
			    if ( !uifont() ) { pErrMsg("no uifont!"); return; }
			    T::setFont( uifont()->qFont() );
			}

    virtual void	fontChange( const QFont& oldFont )
			{
			    uiBody::fontchanged();
			}

    void		closeEvent( QCloseEvent *e ) override
			{
			    if ( uiCloseOK() )
				T::closeEvent(e);
			}

protected:
    uiObject&		uiObjHandle() override		{ return handle_; }
    C&			handle_;
};


/*!\brief uiObjBodyImpl based on uiObjectBody2 (adds shrunk_ without ABI break).
*/

template <class C, class T>
mClass(uiBase) uiObjBodyImpl2 : public uiObjectBody2, public T
{
public:
			uiObjBodyImpl2( C& hndle, uiParent* parnt,
					const char* nm )
			    : uiObjectBody2( parnt, nm )
			    , T( parnt && parnt->pbody() ?
				     parnt->pbody()->managewidg() : nullptr )
			    , handle_( hndle )
			{
			    this->setObjectName( nm );
			}
			~uiObjBodyImpl2()
			{}
			mOD_DisableCopy(uiObjBodyImpl2)

    const QWidget*	qwidget_() const override { return this; }
    virtual void	setFont( const QFont& )
			{
			    if ( !uifont() ) { pErrMsg("no uifont!"); return; }
			    T::setFont( uifont()->qFont() );
			}

    virtual void	fontChange( const QFont& oldFont )
			{
			    uiBody::fontchanged();
			}

    void		closeEvent( QCloseEvent *e ) override
			{
			    if ( uiCloseOK() )
				T::closeEvent(e);
			}

protected:
    uiObject&		uiObjHandle() override		{ return handle_; }
    C&			handle_;
};
