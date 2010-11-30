#ifndef callback_h
#define callback_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		8-11-1995
 Contents:	Notification and Callbacks
 RCS:		$Id: callback.h,v 1.44 2010-11-30 20:39:16 cvskris Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include <string>

/*!
In any OO system callbacks to an unknown client must be possible. To be able
to do this in for class instances, in C++ the called functions need to be:
1) member of a certain pre-defined base class
2) pre-specified in terms of arguments
The following stuff makes sure that there is a nice empty base class with that
role. And, the Capsule mechanism ensures that any class can be passed as
argument.

There are some analogies with QT's signal/slot mechanism. We think our
mechanism is more flexible in some ways, less in other ways (those we're not
interested in).

*/


struct CallBacker { virtual ~CallBacker() {} };


typedef void (CallBacker::*CallBackFunction)(CallBacker*);
/*!> Macro casting a to CallBacker::function */
#define mCBFn(clss,fn) ((CallBackFunction)(&clss::fn))
/*!> Macro to simply define a callback from an instance pointer and a method */
#define mCB(obj,clss,fn) CallBack( static_cast<clss*>(obj), mCBFn(clss,fn))

typedef void (*StaticCallBackFunction)(CallBacker*);
/*!> Macro casting a to StaticCallBacker::function */
#define mSCBFn(clss,fn) ((StaticCallBackFunction)(&clss::fn))
#define mSCB(clss,fn) CallBack( mSCBFn(clss,fn))

/*!\brief CallBacks object-oriented.

CallBack is simply a function pointer + optionally an object to call it on. It
may be empty, in which case doCall() will simply do nothing. If you want to be
able to send a CallBack, you must provide a 'sender' CallBacker* (usually
'this').

*/

mClass CallBack
{
public:
			CallBack( CallBacker* o=0, CallBackFunction f=0 )
			    : obj_( o ), fn_( f ), sfn_( 0 )	{}
			CallBack( StaticCallBackFunction f )
			    : obj_( 0 ), fn_( 0 ), sfn_( f )	{}
    inline int		operator==( const CallBack& cb ) const
			{
			    return obj_ == cb.obj_ &&
				   fn_ == cb.fn_ &&
				   sfn_==cb.sfn_;
			}
    inline int		operator!=( const CallBack& cb ) const
			{ return !(*this==cb); }

    inline bool		willCall() const
			{ return obj_ && fn_ || sfn_; }
    inline void		doCall( CallBacker* o )
			{
			    if ( obj_ && fn_ )
				(obj_->*fn_)( o );
			    else if ( sfn_ )
				sfn_( o );
			}

    inline CallBacker*			cbObj()			{ return obj_; }
    inline const CallBacker*		cbObj() const		{ return obj_; }
    inline CallBackFunction		cbFn() const		{ return fn_; }
    inline StaticCallBackFunction	scbFn() const		{ return sfn_; }

protected:

    CallBacker*			obj_;
    CallBackFunction		fn_;
    StaticCallBackFunction	sfn_;
};


/*!\brief TypeSet of CallBacks with a few extras. */

mClass CallBackSet : public TypeSet<CallBack>
{
public:

    void	doCall(CallBacker*,const bool* enabledflag=0,
	    		CallBacker* exclude=0);
    		/*!<\param enabledflag
		  If enabledflag points to a bool (i.e. is non-zero) that bool
		  will be checked between each call. If the bool is false, the
		  traverse will stop and the remaining cbs won't be called.
		  This makes it possible to terminate a traverse during the
		  traversal.  */
    void	removeWith(CallBacker*);
    		//!< Removes callbacks to this caller
};

inline void CallBackSet::doCall( CallBacker* obj,
				 const bool* enabledflag,
       				 CallBacker* exclude )
{
    const bool enabled_ = true;
    const bool& enabled = enabledflag ? *enabledflag : enabled_;
    if ( !enabled ) return;

    TypeSet<CallBack> cbscopy = *this;
    for ( int idx=0; idx<cbscopy.size(); idx++ )
    {
	CallBack& cb = cbscopy[idx];
	if ( indexOf(cb)==-1 )
	    continue;

	if ( cb.cbObj() != exclude )
	    cb.doCall( obj );
    }
}


inline void CallBackSet::removeWith( CallBacker* cbrm )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.cbObj() == cbrm )
	    { remove( idx ); idx--; }
    }
}


/*!\brief Capsule class to wrap any class into a CallBacker.

Callback functions are defined as:
void clss::func( CallBacker* )
Sometimes you want to pass other info. For this purpose, you can use the
CBCapsule class, which isA CallBacker, but contains T data. For convenience,
the originating CallBacker* is included, so the 'caller' will still be
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

mClass NotifierAccess
{

    friend class	NotifyStopper;

public:

			NotifierAccess()
			    : enabled_(true)			{}
    virtual		~NotifierAccess()			{}

    virtual void	notify(const CallBack&)			=0;
    virtual void	notifyIfNotNotified(const CallBack&)	=0;
    virtual void	remove(const CallBack&)			=0;

    bool		enable( bool newstatus=true )
    			{ return doEnable(newstatus); }
    			/*!<\return previous status */
    bool		disable()		{ return doEnable(false); }
    			/*!<\return previous status */
    bool		isEnabled() const	{ return enabled_; }

protected:

    bool		enabled_;
    inline bool		doEnable( bool newstatus=true )
    			{ bool res=enabled_; enabled_=newstatus; return res; }
    			/*!<\return previous status */
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

mClass NamedNotifierSet
{
public:
				~NamedNotifierSet()
				{ deepErase( names_ ); }

    void			add(const char* nm,NotifierAccess&);
    NotifierAccess*		find(const char*) const;

protected:

    ObjectSet<NotifierAccess>	notifs_;
    ObjectSet<std::string>	names_;

};


inline void NamedNotifierSet::add( const char* nm, NotifierAccess& na )
{
    names_ += new std::string( nm );
    notifs_ += &na;
}


inline NotifierAccess* NamedNotifierSet::find( const char* nm ) const
{
    for ( int idx=0; idx<names_.size(); idx++ )
	if ( *names_[idx] == nm )
	    return const_cast<NotifierAccess*>( notifs_[idx] );
    return 0;
}


/*!\brief implementation class for Notifier */

mClass i_Notifier : public NotifierAccess
{
public:

    virtual void	notify( const CallBack& cb )	{ cbs_ += cb; }
    virtual void	notifyIfNotNotified( const CallBack& cb )
			{ if ( cbs_.indexOf(cb)==-1 ) notify(cb); }
    virtual void	remove( const CallBack& cb )	{ cbs_ -= cb; }
    virtual void	removeWith(CallBacker*);

    CallBackSet		cbs_;
    CallBacker*		cber_;

			i_Notifier()	{}
};


inline void i_Notifier::removeWith( CallBacker* cb )
{
    if ( cber_ == cb )
	{ cbs_.erase(); cber_ = 0; return; }

    cbs_.removeWith( cb );
}


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

The notification can be temporary stopped using disable()/enable() pair,
or by use of a NotifyStopper, which automatically restores the callback
when going out of scope.

*/

template <class T>
class Notifier : public i_Notifier
{
public:

    void		trigger( T& t )	{ trigger(&t); }

// protected: (should be used by T class only)

			Notifier( T* c ) 			{ cber_ = c; }

    inline void		trigger( CallBacker* c=0, CallBacker* exclude=0 )
			{ cbs_.doCall(c ? c : cber_, &enabled_, exclude); }

};


/*!\brief temporarily disables a notifier

CallBacks can be disabled. To do that temporarily, use NotifyStopper.
If the Stopper goes out of scope, the callback is re-enabled. like:

void xxx:doSomething()
{
    NotifyStopper stopper( a_notifier );

    // doing things that would otherwise trigger notifier

    // On exit, notifier gets re-enabled automatically
}

*/

mClass NotifyStopper 
{
public:
			NotifyStopper( NotifierAccess& n ) 
			    : thenotif(n)
			    , oldstatus( n.doEnable(false) )
			{}

    inline		~NotifyStopper()
    			{ restore(); }

    inline void		enable()		{ thenotif.doEnable(false); }
    inline void		disable()		{ thenotif.doEnable(true); }
    inline void		restore()		{ thenotif.doEnable(oldstatus);}

protected:

    NotifierAccess& 	thenotif;
    bool		oldstatus;
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
public:

    void		trigger( C c, T& t )		{ trigger(c,&t); }

// almost protected (as above):

			CNotifier( T* cb )	{ cber_ = cb; }

    inline void		trigger( CallBacker* cb=0 )
			{
			    if( !enabled_ ) return; 
			    C c;
			    trigger(c,cb);
			}

    inline void		trigger( C c, CallBacker* cb=0 )
			{
			    if( !enabled_ ) return; 
			    CBCapsule<C> caps( c, cb ? cb : cber_ );
			    cbs_.doCall( &caps, &enabled_ );
			}
};



#endif
