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
				     parnt->pbody()->managewidg() : 0 )
			    , handle_( hndle )
			{
			    this->setObjectName( nm );
			}
			~uiObjBodyImpl()
			{}

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
