#ifndef callback_H
#define callback_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		8-11-1995
 Contents:	Callbacks for any CallBackClass
 RCS:		$Id: callback.h,v 1.3 2000-05-27 16:22:48 bert Exp $
________________________________________________________________________

You must define the CallBackClass before including this header file. This can
be done on the compiler command line ( ... -DCallBackClass=MyClass ... ) or
simply by defining it with #define .

If you don't define it yourself, you'll get the dGB CallBackClass which is the
infamous UserIDObject.

-*/

#include <sets.h>

#ifndef CallBackClass

# define CallBackClass UserIDObject

#endif


class CallBackClass;


typedef void (CallBackClass::*CallBackFunction)(CallBackClass*);
#define mCBFn(clss,fn) ((CallBackFunction)(&clss::fn))
#define mCB(obj,clss,fn) CallBack(obj,mCBFn(clss,fn))


class CallBack
{
    friend class	CallBackClass;

public:
			CallBack( CallBackClass* o=0, CallBackFunction f=0 )
			{ obj = o; fn = f; }
    int			operator==( const CallBack& cb ) const
			{ return obj == cb.obj && fn == cb.fn; }
    int			operator!=( const CallBack& cb ) const
			{ return obj != cb.obj || fn != cb.fn; }

    void		doCall( CallBackClass* o )
			{ if ( obj && fn ) (obj->*fn)( o ); }
    bool		willCall() const
			{ return obj && fn ? YES : NO; }

protected:

    CallBackClass*	obj;
    CallBackFunction	fn;

};

class CallBackList : public TypeSet<CallBack>
{
public:
    void	doCall(CallBackClass*);
};

inline void CallBackList::doCall( CallBackClass* obj )
{
    for ( int idx=0; idx<size(); idx++ )
	(*this)[idx].doCall( obj );
}


#endif
