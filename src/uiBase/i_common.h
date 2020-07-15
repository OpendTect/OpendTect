#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "q_uiimpl.h"
#include "uiobjbody.h"


/*!\brief Default (Template) implementation of uiObjectBody.


    Any uiObjectBody must implement thiswidget_() and uiObjHandle()
    and must also implement some QWidget methods.
    These are implemented using the pre-processor, because it is difficult
    to templatize for all cases since some Qt objects need a specific
    constructor.
    However, most QWidgets just need a parent and a name
    and then this template implementation can be used.

    \code

    #define mHANDLE_OBJ		uiObject_or_child_class, default O
    #define mQWIDGET_BODY       Widget_or_child_class, default T
    #define mQWIDGET_BASE       QWidget
    #include			"i_uiobjqtbody.h"

    \endcode

    The macro's mQWIDGET_BODY and mQWIDGET_BASE are undef'ed
    in "i_uiobjqtbody.h"

    The "i_uiobjqtbody.h" header file uses no multiple-inclusion protection
    and implements the following methods:

    \code

	virtual QWidget*	qwidget_();
	virtual void		setFont( const QFont &font );
	virtual void		closeEvent( QCloseEvent *e );
	virtual void		polish();

    protected:

	virtual uiObject&	uiObjHandle();

    \endcode

    It also declares a protected member handle_, of type mHANDLE_OBJ&, which
    must be initialized in any constructor of objects using "i_uiobjqtbody.h"

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

#include		"i_uiobjqtbody.h"

};
