#ifndef callback_H
#define callback_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		8-11-1995
 Contents:	Callbacks for any user ID object
 RCS:		$Id: callback.h,v 1.2 2000-05-26 10:37:03 bert Exp $
________________________________________________________________________

-*/

#include <sets.h>
class UserIDObject;


typedef void (UserIDObject::*InformFunction)(UserIDObject*);
#define mCBFn(clss,fn) ((InformFunction)(&clss::fn))
#define mCB(obj,clss,fn) CallBack(obj,mCBFn(clss,fn))


class CallBack
{
    friend class	UserIDObject;

public:
			CallBack( UserIDObject* o=0, InformFunction f=0 )
			{ obj = o; fn = f; }
    int			operator==( const CallBack& cb ) const
			{ return obj == cb.obj && fn == cb.fn; }
    int			operator!=( const CallBack& cb ) const
			{ return obj != cb.obj || fn != cb.fn; }

    void		doCall( UserIDObject* o )
			{ if ( obj && fn ) (obj->*fn)( o ); }
    int			willCall() const
			{ return obj && fn ? YES : NO; }

protected:

    UserIDObject*	obj;
    InformFunction	fn;

};

class CallBackList : public TypeSet<CallBack>
{
public:
    void	doCall(UserIDObject*);
};

inline void CallBackList::doCall( UserIDObject* obj )
{
    for ( int idx=0; idx<size(); idx++ )
	(*this)[idx].doCall( obj );
}


/*$-*/
#endif
