#ifndef callback_H
#define callback_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		8-11-1995
 Contents:	Callbacks for any CallBacker
 RCS:		$Id: callback.h,v 1.11 2000-09-22 10:07:14 bert Exp $
________________________________________________________________________

-*/

#ifndef CallBacker

#   define CallBacker CallBackClass
    class CallBackClass
    { public: virtual void _a_dummy_virtual_enabling_dynamic_cast_() {} };

#else

    class CallBacker;

#endif


typedef void (CallBacker::*CallBackFunction)(CallBacker*);
#define mCBFn(clss,fn) ((CallBackFunction)(&clss::fn))
#define mCB(obj,clss,fn) CallBack(obj,mCBFn(clss,fn))


/*! \brief CallBacks object-oriented.

If you want a specific class to be the 'callback base class', you must define
the CallBacker before including this header file. This can
be done on the compiler command line ( ... -DCallBacker=MyClass ... ) or
simply by defining it with #define .

If you don't define it yourself, you'll get the dGB CallBackClass which
is (almost) empty.

A simple list of CallBacks is also required. Again, define CallBackList if
needed. In this case, you'll need to support:

1) An 'operator +=' to add a callback to the list
2) An 'operator -=' to remove a callback from the list
3) An 'operator [](int)' const that returns the i-th callback in the list.
4) A method 'size() const' that returns the number of callbacks in the list
5) A method 'doCall(CallBacker*)' that simply calls all the CallBacks' doCall
   methods.

If you do not define CallBackList you will get dGB's CallBackSet, which is
based on a simple dumbed-down implementation a bit like the std lib's vector
class.

*/

class CallBack
{
public:
			CallBack( CallBacker* o=0, CallBackFunction f=0 )
			{ obj = o; fn = f; }
    int			operator==( const CallBack& cb ) const
			{ return obj == cb.obj && fn == cb.fn; }
    int			operator!=( const CallBack& cb ) const
			{ return obj != cb.obj || fn != cb.fn; }

    bool		willCall() const
			{ return obj && fn; }
    void		doCall( CallBacker* o )
			{ if ( obj && fn ) (obj->*fn)( o ); }

    CallBacker*		cbObj()			{ return obj; }
    const CallBacker*	cbObj() const		{ return obj; }
    CallBackFunction	cbFn() const		{ return fn; }

protected:

    CallBacker*		obj;
    CallBackFunction	fn;

};


#ifndef CallBackList


#define CallBackList CallBackSet
#include <sets.h>

class CallBackSet : public TypeSet<CallBack>
{
public:

    void	doCall(CallBacker*);

};

inline void CallBackSet::doCall( CallBacker* obj )
{
    for ( int idx=0; idx<size(); idx++ )
	(*this)[idx].doCall( obj );
}


#endif


/*! \brief A Capsule class to wrap any class into a CallBacker.

for convenience, a CallBacker* is included, so the 'caller' will still be
available.
*/

template <class T>
class CBCapsule : public CallBacker
{
public:
			CBCapsule( T d, CallBacker* c )
			: data(d), caller(c)	{}

    T			data;
    CallBacker*		caller;
};

/*! \brief Unpack data from capsule

If you have a pointer to a capsule cb, this:
\code
    mCBCapsuleUnpack(const uiMouseEvent&,ev,cb)
\endcode
would result in the availability of:
\code
    const uiMouseEvent& ev
\endcode

*/

#define mCBCapsuleUnpack(T,var,cb) \
    CBCapsule<T>* cb##caps = dynamic_cast< CBCapsule<T>* >( cb ); \
    T var = cb##caps->data


/*! \brief Notifier classes help setup a callback handling.

Simply declare a Notifier<T> in the interface, like:
\code
Notifier<MyClass>	buttonclicked;
\endcode

Then users of the class can issue:

\code
aMyClass.buttonclicked.notify( mCB(this,TheClassOfThis,TheMethodToBeCalled) );
\endcode

The callback is issued when you call the trigger() method.
*/

class i_Notifier
{
public:

    void		notify( const CallBack& cb )	{ cbs += cb; }

protected:

    CallBackList	cbs;
    CallBacker*		cber;

			i_Notifier()	{}

};


template <class T>
class Notifier : public i_Notifier
{
    friend		T;

public:

    void		trigger( T& t )		{ trigger(&t); }

protected:

			Notifier( T* c )	{ cber = c; }

    inline void		trigger(CallBacker* c=0){ cbs.doCall(c ? c : cber); }

};


/*! \brief Notifier with automatic capsule creation.

When non-callbacker data needs to be passed, you can put it in a capsule.

You'll need to define:

\code
Notifier<MyClass,const uiMouseEvent&>	mousepress;
\endcode

*/

template <class T,class C>
class CNotifier : public i_Notifier
{
    friend		T;

public:

    void		trigger( C c, T& t )		{ trigger(c,&t); }

protected:

			CNotifier( T* cb )	{ cber = cb; }

    inline void		trigger( C c=0, CallBacker* cb=0 )
			{
			    CBCapsule<C> caps( c, cb ? cb : cber );
			    cbs.doCall( &caps );
			}

};



#endif
