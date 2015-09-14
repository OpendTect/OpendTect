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
#include "refcount.h"
#include "sets.h"
#include "threadlock.h"

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
    static void		initClass();

			CallBack( CallBacker* o=0, CallBackFunction f=0 )
			    : obj_( o ), fn_( f ), sfn_( 0 )	{}
			CallBack( StaticCallBackFunction f )
			    : obj_( 0 ), fn_( 0 ), sfn_( f )	{}
    inline int		operator==( const CallBack& c ) const
			{ return obj_==c.obj_ && fn_==c.fn_ && sfn_==c.sfn_; }
    inline int		operator!=( const CallBack& cb ) const
			{ return !(*this==cb); }

    inline bool		willCall() const
			{ return (obj_ && fn_) || sfn_; }
    void		doCall(CallBacker*) const;

    inline CallBacker*			cbObj()			{ return obj_; }
    inline const CallBacker*		cbObj() const		{ return obj_; }
    inline CallBackFunction		cbFn() const		{ return fn_; }
    inline StaticCallBackFunction	scbFn() const		{ return sfn_; }

    static bool		addToMainThread(const CallBack&, CallBacker* =0);
                        /*!< Unconditionally add this to main event loop.
                         For thread safety, the removeFromMainThread()
                         must be called in the destructor. */

    static bool		callInMainThread(const CallBack&, CallBacker* =0);
                        /*!<If in main thread or no event-loop is present, it
                            will be called directly. Otherwise, it will be
                            put on event loop.
                            For thread safety, the removeFromMainThread()
                            must be called in the destructor.
                            \returns true if the callback was called directly.
                        */
    static void		removeFromMainThread(const CallBacker*);


    // See also mEnsureExecutedInMainThread macro

protected:

    CallBacker*				obj_;
    CallBackFunction			fn_;
    StaticCallBackFunction		sfn_;

public:

    // Usually only called from mEnsureExecutedInMainThread:

    static bool				queueIfNotInMainThread(CallBack,
							CallBacker* =0);
					/*!< If not in main thread, queue it.
					   return whether CB was queued. */

};

#define mMainThreadCall( func ) \
CallBack::callInMainThread( CallBack( this, ((CallBackFunction)(&func) ) ), 0)

#define mEnsureExecutedInMainThread( func ) \
    if ( CallBack::queueIfNotInMainThread( \
	CallBack( this, ((CallBackFunction)(&func) ) ), 0 ) )  \
	return


/*!
\brief TypeSet of CallBacks with a few extras.
*/

mExpClass(Basic) CallBackSet : public TypeSet<CallBack>
{ mRefCountImpl(CallBackSet)
public:
		CallBackSet();
		CallBackSet(const CallBackSet&);
    CallBackSet& operator=(const CallBackSet&);

    void	doCall(CallBacker*,CallBacker* exclude=0);
		/*!<\param enabledflag: if non-null, content will be checked
		  between each call, caling will stop if false.
		     \note Will lock in the apropriate moment. */

    void	removeWith(CallBacker*);
		//!<\note Should be locked before calling
    void	removeWith(CallBackFunction);
		//!<\note Should be locked before calling
    void	removeWith(StaticCallBackFunction);
		//!<\note Should be locked before calling

    inline bool	isEnabled() const { return enabled_; }
    bool	doEnable( bool yn=true );
    		//!<Returns old state

    mutable Threads::Lock   	lock_;
private:
    bool			enabled_;
};


/*!
\brief Interface class for Notifier. See comments there.
*/

mExpClass(Basic) NotifierAccess
{

    friend class	NotifyStopper;
    friend class	CallBacker;

public:

			NotifierAccess(const NotifierAccess&);
			NotifierAccess();
    virtual		~NotifierAccess();

    void		notify(const CallBack&,bool first=false);
    bool		notifyIfNotNotified(const CallBack&);
			//!\returns true if it was added
    void		remove(const CallBack&);
    bool		removeWith(CallBacker*,bool wait=true);
			//!<\returns false only if wait and no lock could be got

    inline bool	isEnabled() const	{ return cbs_.isEnabled(); }
    inline bool	enable( bool yn=true )	{ return cbs_.doEnable(yn); }
    inline bool	disable()		{ return cbs_.doEnable(false); }

    inline bool	isEmpty() const 	{ return cbs_.isEmpty(); }
    bool		willCall(CallBacker*) const;
			/*!<\returns true if the callback list contains
			     CallBacker. */


    CallBackSet&	cbs_;
    CallBacker*		cber_;

    bool		isShutdownSubscribed(CallBacker*) const;
			//!<Only for debugging purposes, don't use
protected:
    static void		doTrigger(CallBackSet&,CallBacker* c,
	    			  CallBacker* exclude);
    void		addShutdownSubscription(CallBacker*);
    bool		removeShutdownSubscription(CallBacker*, bool wait);
			//!<\returns false only if wait and no lock could be got

			/*!\returns previous status */

    ObjectSet<CallBacker>	shutdownsubscribers_;
    mutable Threads::Lock	shutdownsubscriberlock_;
};


/*!
  Class to help setup a callback handling.

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
  mAttachCB( myclass.buttonClicked, TheClassOfThis::theMethodToBeCalle );
  \endcode

  The notifier is then attached, the connection will be remove when either the
  notifier or the called object is deleted.

  The callback is issued when you call the trigger() method, like:
  \code
  buttonClicked.trigger();
  \endcode

  The notification can be temporary stopped using disable()/enable() pair,
  or by use of a NotifyStopper, which automatically restores the callback
  when going out of scope.

  The best practice is to remove the callbacks in the destructor, as otherwise,
  you may get random crashes. Either, remove them one by one in the destructor,
  or call detachAllNotifiers(), which will remove notifiers that are attached
  using the mAttachCB macro.
*/


template <class T>
mClass(Basic) Notifier : public NotifierAccess
{
public:

    void		trigger( T& t )	{ trigger(&t); }

			// Following functions are usually used by T class only:
			Notifier( T* c )			{ cber_ = c; }

    inline void		trigger( CallBacker* c=0, CallBacker* exclude=0 )
			{ doTrigger( cbs_, c ? c : cber_, exclude ); }
};


/*!
\brief To be able to send and/or receive CallBacks, inherit from this class.
*/

mExpClass(Basic) CallBacker
{
    friend class	NotifierAccess;
public:
			CallBacker();
			CallBacker(const CallBacker&);
    virtual		~CallBacker();

    bool		attachCB(NotifierAccess&,const CallBack&,
				 bool onlyifnew=false);
			/*!<Adds cb to notifier, and makes sure
			    it is removed later when object is
			    deleted.
			    \returns if it was attached. */
    bool		attachCB(NotifierAccess* notif,const CallBack& cb,
				 bool onlyifnew=false)
			{ return notif ? attachCB(*notif,cb,onlyifnew):false; }
			/*!<\note Attaches only if \param notif is not null.*/

    void		detachCB(NotifierAccess&,const CallBack&);
			/*!<\note Normally not needed if you don't
			          want this explicitly. */
    void		detachCB(NotifierAccess* notif,const CallBack& cb)
			{ if ( notif ) detachCB( *notif, cb ); }
			/*!<\note Detaches only if \param notif is not null.*/

    bool		isNotifierAttached(NotifierAccess*) const;
			//!<Only for debugging purposes, don't use

protected:
    void		detachAllNotifiers();
			//!<Call from the destructor of your inherited object
private:

    bool		notifyShutdown(NotifierAccess*,bool wait);
			//!<\returns false only if wait and no lock could be got

    ObjectSet<NotifierAccess>	attachednotifiers_;
    mutable Threads::Lock	attachednotifierslock_;
};


#define mAttachCB( notifier, func ) \
attachCB( notifier, CallBack( this, ((CallBackFunction)(&func) ) ), false )

#define mAttachCBIfNotAttached( notifier, func ) \
attachCB( notifier, CallBack( this, ((CallBackFunction)(&func) ) ), true )

#define mDetachCB( notifier, func ) \
detachCB( notifier, CallBack( this, ((CallBackFunction)(&func) ) ) )

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
mClass(Basic) CBCapsule : public CallBacker
{
public:
    CBCapsule( T d, CallBacker* c )
    : data(d), caller(c)	{}

    T			data;
    CallBacker*		caller;
};


/*!
\ingroup Basic
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
mClass(Basic) CNotifier : public NotifierAccess
{
public:

    void		trigger( C c, T& t )		{ trigger(c,&t); }

// Following functions are usually used by T class only:

			CNotifier( T* cb )	{ cber_ = cb; }

    inline void		trigger( C c, CallBacker* cb=0 )
			{
			    CBCapsule<C> caps( c, cb ? cb : cber_ );
			    doTrigger( cbs_, &caps, 0 );
			}
};


/*!
\brief Temporarily disables a Notifier.

  Notifiers can be disabled. To do that temporarily, use NotifyStopper.
  If the Stopper goes out of scope, the callback is re-enabled. like:

  \code
  void xxx:doSomething()
  {
      NotifyStopper stopper( a_notifier );
      // Doing things that would otherwise trigger Notifier.
      // On exit, Notifier gets re-enabled automatically.
  }
  \endcode
*/

mExpClass(Basic) NotifyStopper
{
public:
		NotifyStopper( NotifierAccess& na );
    		~NotifyStopper();

    void        enable();    
    void        disable();    
    void	restore();

protected:

    NotifierAccess&	thenotif_;
    bool		oldst_;

};


// Set of macros to add an instanceCreated() notifier
// This can provide a notification of any instance of a class being produced

#define mDeclInstanceCreatedNotifierAccess(clss) \
    static Notifier<clss>&	instanceCreated()

#define mDefineInstanceCreatedNotifierAccess(clss) \
Notifier<clss>& clss::instanceCreated() \
{ \
    mDefineStaticLocalObject( Notifier<clss>, theNotif, (0)); \
    return theNotif; \
}

#define mTriggerInstanceCreatedNotifier() \
    instanceCreated().trigger( this )


#endif

