#ifndef callback_h
#define callback_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		8-11-1995
 Contents:	Notification and Callbacks
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "sets.h"
#include "thread.h"
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

class CallBacker;


typedef void (CallBacker::*CallBackFunction)(CallBacker*);
#define mCBFn(clss,fn) ((CallBackFunction)(&clss::fn))

//!> To make your CallBack. Used in many places, especially the UI.
#define mCB(obj,clss,fn) CallBack( static_cast<clss*>(obj), mCBFn(clss,fn))

typedef void (*StaticCallBackFunction)(CallBacker*);
#define mSCB(fn) CallBack( ((StaticCallBackFunction)(&fn)) )


/*!
\brief CallBacks object-oriented (object + method).
  
  CallBack is nothing more than a function pointer + optionally an object to
  call it on. It may be null, in which case doCall() will simply do nothing.
  If you want to be able to send a CallBack, you must provide a 'sender'
  CallBacker* (usually 'this').
*/

mExpClass(Basic) CallBack
{
public:
			CallBack( CallBacker* o=0, CallBackFunction f=0 )
			    : obj_( o ), fn_( f ), sfn_( 0 )	{}
			CallBack( StaticCallBackFunction f )
			    : obj_( 0 ), fn_( 0 ), sfn_( f )	{}
    inline int		operator==( const CallBack& c ) const
			{ return obj_==c.obj_ && fn_==c.fn_ && sfn_==c.sfn_; }
    inline int		operator!=( const CallBack& cb ) const
			{ return !(*this==cb); }

    inline bool		willCall() const
			{ return obj_ && (fn_ || sfn_); }
    void		doCall(CallBacker*);

    inline CallBacker*			cbObj()			{ return obj_; }
    inline const CallBacker*		cbObj() const		{ return obj_; }
    inline CallBackFunction		cbFn() const		{ return fn_; }
    inline StaticCallBackFunction	scbFn() const		{ return sfn_; }

protected:

    CallBacker*				obj_;
    CallBackFunction			fn_;
    StaticCallBackFunction		sfn_;

};


/*!
\brief TypeSet of CallBacks with a few extras.
*/

mExpClass(Basic) CallBackSet : public TypeSet<CallBack>
{
public:

    void	doCall(CallBacker*,const bool* enabledflag=0,
	    		CallBacker* exclude=0);
    		/*!<\param enabledflag: if non-null, content will be checked
		  between each call, caling will stop if false. */

    void	removeWith(CallBacker*);
    void	removeWith(CallBackFunction);
    void	removeWith(StaticCallBackFunction);

};


/*!
\brief Interface class for Notifier. See comments there.
*/

mExpClass(Basic) NotifierAccess
{

    friend class	NotifyStopper;

public:

			NotifierAccess();
    
    virtual void	notify(const CallBack&,bool first=false);
    virtual void	notifyIfNotNotified(const CallBack&);
    virtual void	remove(const CallBack&);
    virtual void	removeWith(CallBacker*);

    bool		isEnabled() const	{ return enabled_; }
    bool		enable( bool yn=true )	{ return doEnable(yn); }
    bool		disable()		{ return doEnable(false); }
    
    CallBackSet		cbs_;
    CallBacker*		cber_;

protected:

    bool		enabled_;
    inline bool		doEnable( bool yn=true )
    			{ bool ret = enabled_; enabled_ = yn; return ret; }
    			/*!< returns previous status */
};


/*!
\brief Class to help setup a callback handling.
  
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
  Notifier<MyClass>	buttonClicked;
  \endcode
  
  Then users of the class can issue:
  
  \code
  amyclass.buttonClicked.notify( mCB(this,TheClassOfThis,theMethodToBeCalled) );
  \endcode
  
  The callback is issued when you call the trigger() method, like:
  \code
  buttonClicked.trigger();
  \endcode
  
  The notification can be temporary stopped using disable()/enable() pair,
  or by use of a NotifyStopper, which automatically restores the callback
  when going out of scope.  
*/

template <class T>
class Notifier : public NotifierAccess
{
public:

    void		trigger( T& t )	{ trigger(&t); }

// Following functions are usually used by T class only:

			Notifier( T* c ) 			{ cber_ = c; }

    inline void		trigger( CallBacker* c=0, CallBacker* exclude=0 )
			{ cbs_.doCall(c ? c : cber_, &enabled_, exclude); }

};


/*!
\brief To be able to send and/or receive CallBacks, inherit from this class.
*/

mExpClass(Basic) CallBacker
{
public:
				CallBacker();
				CallBacker(const CallBacker&);
    virtual 			~CallBacker();
    
    void			attachCB(NotifierAccess&,const CallBack&);
    				/*!<Adds cb to notifier, and makes sure
				    it is removed later when object is
				    deleted. */
    void			detachCB(NotifierAccess&,const CallBack&);
    				/*!<\note Normally not needed if you don't
				          Want this explicitly. */
    
private:
    void			removeListener(CallBacker*);
    void			addListener(CallBacker*);
    ObjectSet<CallBacker>	listeners_;
    
    ObjectSet<NotifierAccess>	attachednotifiers_;
    Threads::SpinLock		cblock_;
};


#define mAttachCB( notifier, clss, func ) \
attachCB( notifier, mCB(this,clss,func) )

#define mDetachCB( notifier, clss, func ) \
detachCB( notifier, clss, func )


/*!
\brief Capsule class to wrap any class into a CallBacker.
  
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


/*!
\brief Unpacking data from capsule.
  
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


/*!
\brief Notifier with automatic capsule creation.
  
  When non-callbacker data needs to be passed, you can put it in a capsule.
  
  You'll need to define:
  
  \code
  CNotifier<MyClass,const uiMouseEvent&>	mousepress;
  \endcode  
*/

template <class T,class C>
class CNotifier : public NotifierAccess
{
public:

    void		trigger( C c, T& t )		{ trigger(c,&t); }

// Following functions are usually used by T class only:

			CNotifier( T* cb )	{ cber_ = cb; }

    inline void		trigger( CallBacker* cb=0 )
			    { if( !enabled_ ) return; C c; trigger(c,cb); }

    inline void		trigger( C c, CallBacker* cb=0 )
			{
			    if ( enabled_ )
			    {
				CBCapsule<C> caps( c, cb ? cb : cber_ );
				cbs_.doCall( &caps, &enabled_ );
			    }
			}
};


/*!
\brief Temporarily disables a Notifier.
  
  Notifiers can be disabled. To do that temporarily, use NotifyStopper.
  If the Stopper goes out of scope, the callback is re-enabled. like:
  
  void xxx:doSomething()
  {
      NotifyStopper stopper( a_notifier );
      // Doing things that would otherwise trigger Notifier.
      // On exit, Notifier gets re-enabled automatically.
  }
*/

mExpClass(Basic) NotifyStopper 
{
public:
			NotifyStopper( NotifierAccess& na ) 
			    : oldst_(na.doEnable(false))
			    , thenotif_(na)	{}

    inline		~NotifyStopper()	{ restore(); }

    inline void		enable()		{ thenotif_.doEnable(false); }
    inline void		disable()		{ thenotif_.doEnable(true); }
    inline void		restore()		{ thenotif_.doEnable(oldst_);}

protected:

    NotifierAccess& 	thenotif_;
    bool		oldst_;

};


// Set of macros to add an instanceCreated() notifier
// This can provide a notification of any instance of a class being produced

#define mDeclInstanceCreatedNotifierAccess(clss) \
    static Notifier<clss>&	instanceCreated()

#define mDefineInstanceCreatedNotifierAccess(clss) \
Notifier<clss>& clss::instanceCreated() \
{ \
    static Notifier<clss> theNotif(0); \
    return theNotif; \
}

#define mTriggerInstanceCreatedNotifier() \
    instanceCreated().trigger( this )


#endif

