#ifndef callback_H
#define callback_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		8-11-1995
 Contents:	Callbacks for any CallBacker
 RCS:		$Id: callback.h,v 1.22 2002-02-08 11:51:28 bert Exp $
________________________________________________________________________

-*/

#include <gendefs.h>  // only needed for mTFriend and mProtected macros

/*!
In any OO system callbacks to an unknown client must be possible. To be able
to do this in for class instances, in C++ the called functions need to be:
1) member of a certain pre-defined base class
2) pre-specified in terms of arguments
The following stuff tries to make sure that the base class can be chosen,
but a nice 'empty' class is provided here. And, the Capsule mechanism ensures
that any class can be passed as argument.

There are some analogies with QT's signal/slot mechanism. We think our
mechanism is more flexible in some ways, less in other ways (those we're not
interested in).

*/


#ifndef CallBacker

#   define CallBacker CallBackClass
    class CallBackClass
    { public: virtual void _a_dummy_virtual_enabling_dynamic_cast_() {} };

#else

    class CallBacker;

#endif


typedef void (CallBacker::*CallBackFunction)(CallBacker*);
/*!> Macro casting a to CallBacker::function */
#define mCBFn(clss,fn) ((CallBackFunction)(&clss::fn))
/*!> Macro to simply define a callback from an instance pointer and a method */
#define mCB(obj,clss,fn) CallBack(obj,mCBFn(clss,fn))


/*!\brief CallBacks object-oriented.

CallBack is simply a function pointer + object to call it on. It may be
empty, in which case doCall() will simply do nothing. If you want to be
able to send a CallBack, you must be a CallBacker.

\NOTE: If you want a specific class to be the 'callback base class', you must
define the CallBacker before including this header file. This can
be done on the compiler command line ( ... -DCallBacker=MyClass ... ) or
simply by defining it with #define. If you don't define it yourself, you'll
get the dGB CallBackClass which is (almost) empty.

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


/*!\brief List of CallBacks.

A simple list of CallBacks is required. Again, define CallBackList if
needed. In that case, you'll need to support:

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


/*!\brief Capsule class to wrap any class into a CallBacker.

Because callback functions are defined as:
void xxx( CallBacker* )
you can also pass other info. For this reason, you can use the Capsule
class, which isA CallBacker, but contains T data. For convenience, the
originating CallBacker* is included, so the 'caller' will still be available.

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


/*!\brief Unpacking data from capsule

If you have a pointer to a capsule cb, this:
\code
    mCBCapsuleUnpack(const uiMouseEvent&,ev,cb)
\endcode
would result in the availability of:
\code
    const uiMouseEvent& ev
\endcode

If you're interested in the caller, you'll need to get the capsule itself:
\code
    mCBCapsuleGet(const uiMouseEvent&,caps,cb)
\endcode
would result in the availability of:
\code
    CBCapsule<const uiMouseEvent&>* caps
\endcode

*/

#define mCBCapsuleGet(T,var,cb) \
    CBCapsule<T>* var = dynamic_cast< CBCapsule<T>* >( cb );

#define mCBCapsuleUnpack(T,var,cb) \
    mCBCapsuleGet(T,cb##caps,cb) \
    T var = cb##caps->data

#define mCBCapsuleUnpackWithCaller(T,var,cber,cb) \
    mCBCapsuleGet(T,cb##caps,cb) \
    T var = cb##caps->data; \
    CallBacker* cber = cb##caps->caller


/*!\brief interface class for Notifier. See comments there. */

class NotifierAccess
{

friend class		NotifyStopper;

public:

			NotifierAccess() : enabled( true )	{}
    virtual		~NotifierAccess()			{}

    virtual void	notify(const CallBack&)			=0;

protected:

    bool		enabled;
    inline void		enable()				{enabled=true; }
    inline void		disable()				{enabled=false;}

};


/*!\brief List of named Notifier objects.

To be able to set up generalised communication mechanisms based on callbacks,
we'll need to be able to 'publish' a Notifier under a symbolic name.
The list needs to support:

1) void add( const char* name, NotifierAccess* )
2) NotifierAccess* find( const char* name )

No management or whatsoever is supported as this is just a generalised way
to 'publish' event notification abilities.

*/

#ifndef NamedNotifierList

#define NamedNotifierList NamedNotifierSet
#include <sets.h>

class NamedNotifierSet
{
public:
				~NamedNotifierSet()
				{ deepErase( names ); }

    void			add( const char* nm, NotifierAccess& na )
				{ names += new BufferString(nm); notifs += &na;}
    NotifierAccess*		find(const char*) const;

protected:

    ObjectSet<NotifierAccess>	notifs;
    ObjectSet<BufferString>	names;

};

inline NotifierAccess* NamedNotifierSet::find( const char* nm ) const
{
    for ( int idx=0; idx<names.size(); idx++ )
	if ( *names[idx] == nm )
	    return const_cast<NotifierAccess*>( notifs[idx] );
    return 0;
}

#endif


/*!\brief implementation class for Notifier */

class i_Notifier : public NotifierAccess
{
public:

    virtual void	notify( const CallBack& cb )	{ cbs += cb; }

protected:

    CallBackList	cbs;
    CallBacker*		cber;

			i_Notifier()	{}

};


/*!\brief class to help setup a callback handling.

What we have discovered is that the two things:
- providing a notification of an event to the outside world
- asking a notification of a certain object
are strongly coupled. Qt has its Signal/Slot pair, but we found that too
inflexible. Enter Notifier. You declare a Notifier to announce to the
world that they can be notified of something. The 'receiving' object can
then call the notify() method to 'register' the event notification. The
sending object just calls trigger(). Note that it is most used in the
UI, but it is equally usable in batch stuff. In general, it provides a
rigorous uncoupling.

Simply declare a Notifier<T> in the interface, like:
\code
Notifier<MyClass>	buttonclicked;
\endcode

Then users of the class can issue:

\code
amyclass.buttonclicked.notify( mCB(this,TheClassOfThis,theMethodToBeCalled) );
\endcode

The callback is issued when you call the trigger() method, like:
\code
buttonclicked.trigger();
\endcode

The callback can be temporary stopped using disable()/enable() pair,
or by use of a NotifyStopper, which automatically enables the callback
when going out of scope.

*/

template <class T>
class Notifier : public i_Notifier
{
    friend		T;

public:

    void		trigger( T& t )				{ trigger(&t); }
    void		enable( T& t )				{ enable(); }
    void		disable( T& t )				{ disable(); }

protected:

			Notifier( T* c ) 			{ cber = c; }

    inline void		trigger( CallBacker* c=0 )
			{ if ( enabled ) cbs.doCall(c ? c : cber); }

};


/* \brief temporarily disables a notifier

CallBacks can be disabled. To do that temporarily, use NotifyStopper.
If the Stopper goes out of scope, the callback is re-enabled. like:

void xxx:doSomething()
{
    NotifyStopper stopper( a_notifier );

    // doing things that would otherwise trigger notifier

    // On exit, notifier gets re-enabled automatically
}

*/

class NotifyStopper 
{
public:
			NotifyStopper( NotifierAccess& n ) 
			: thenotif(n)		{ enable(); }

    inline		~NotifyStopper()	{ thenotif.enable();}

    inline void		enable()		{ thenotif.disable(); }
    inline void		disable()		{ thenotif.enable(); }

protected:

    NotifierAccess& 	thenotif;

};





/*! \brief Notifier with automatic capsule creation.

When non-callbacker data needs to be passed, you can put it in a capsule.

You'll need to define:

\code
CNotifier<MyClass,const uiMouseEvent&>	mousepress;
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

    inline void		trigger( CallBacker* cb=0 )
			{
			    if( !enabled ) return; 
			    C c;
			    trigger(c,cb);
			}

    inline void		trigger( C c, CallBacker* cb=0 )
			{
			    if( !enabled ) return; 
			    CBCapsule<C> caps( c, cb ? cb : cber );
			    cbs.doCall( &caps );
			}
};



#endif
