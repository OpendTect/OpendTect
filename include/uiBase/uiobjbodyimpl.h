#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2018
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
