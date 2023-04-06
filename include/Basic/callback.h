#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "refcount.h"
#include "sets.h"
#include "threadlock.h"
class NotifierAccess;

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


using CallBackFunction = void(CallBacker::*)(CallBacker*);
#define mCBFn(fn) (static_cast<CallBackFunction>(&fn))

//!> To make your CallBack. Used in many places, especially the UI.
#define mCB(obj,clss,fn) CallBack( static_cast<clss*>(obj), mCBFn(clss::fn))

using StaticCallBackFunction = void(*)(CallBacker*);
#define mSCB(fn) CallBack( (static_cast<StaticCallBackFunction>(&fn)) )


/*!\brief CallBacks object-oriented (object + method).

  CallBack is nothing more than a function pointer + optionally an object to
  call it on. It may be null, in which case doCall() will simply do nothing.
  If you want to be able to send a CallBack, you must provide a 'sender'
  CallBacker* (usually 'this').

  You can disable a CallBack, but if you do make sure you enable it again
  using disable(false), as it works with a counter (so that disabling can be
  nested).

*/

mExpClass(Basic) CallBack
{
public:
    static void		initClass();
			CallBack()
			    : cberobj_(nullptr), fn_(nullptr), sfn_(nullptr)
			{}
			CallBack( CallBacker* o, CallBackFunction f )
			    : cberobj_(o), fn_(f), sfn_(nullptr)
			{}
			CallBack( StaticCallBackFunction f )
			    : cberobj_(nullptr), fn_(nullptr), sfn_(f)
			{}
    bool		operator==(const CallBack&) const;
    bool		operator!=(const CallBack&) const;

    bool		willCall() const;
    void		doCall(CallBacker*) const;
    bool		isDisabled() const		{ return disablecount_;}
    void		disable(bool yn=true) const;
    void		enable() const			{ disable(false); }

    inline CallBacker*			cbObj()		{ return cberobj_; }
    inline const CallBacker*		cbObj() const	{ return cberobj_; }
    inline CallBackFunction		cbFn() const	{ return fn_; }
    inline StaticCallBackFunction	scbFn() const	{ return sfn_; }

    static bool addToMainThread(const CallBack&, CallBacker* =nullptr);
		/*!< Unconditionally add this to main event loop.
		     For thread safety, the removeFromThreadCalls()
		     must be called in the destructor. */
    static bool addToThread(Threads::ThreadID,const CallBack&,
				    CallBacker* = nullptr);
		/*!< Unconditionally add this to event loop of the other thread.
		     For thread safety, the removeFromThreadCalls()
		     must be called in the destructor. */

    static bool callInMainThread(const CallBack&, CallBacker* =nullptr);
		/*!<If in main thread or no event-loop is present, it
		    will be called directly. Otherwise, it will be
		    put on event loop.
		    For thread safety, the removeFromThreadCalls()
		    must be called in the destructor.
		    \returns true if the callback was called directly.
                        */

    static void removeFromThreadCalls(const CallBacker*);
		/* Removes callbacker from all event loops in all threads*/


    // See also mEnsureExecutedInMainThread macro

private:

    CallBacker*				cberobj_;
    CallBackFunction			fn_;
    StaticCallBackFunction		sfn_;
    mutable Threads::Atomic<int>	disablecount_;
    static Threads::ThreadID		mainthread_;

public:

    // Usually only called from mEnsureExecutedInMainThread:

    static bool			queueIfNotInMainThread(CallBack,
						CallBacker* =nullptr);
				/*!< If not in main thread, queue it.
				   return whether CB was queued. */

    mDeprecated("Use removeFromThreadCalls")
		static void removeFromMainThread( const CallBacker* cber )
		{ removeFromThreadCalls( cber ); }
};

#define mMainThreadCall( func ) \
CallBack::callInMainThread( CallBack( this, mCBFn(func) ), 0)

#define mEnsureExecutedInMainThread( func ) \
    if ( CallBack::queueIfNotInMainThread( \
	CallBack( this, mCBFn(func) ), 0 ) )  \
	return

#define mEnsureExecutedInMainThreadWithCapsule( func, caps ) \
    CallBack cb( this, mCBFn(func) ); \
    if ( CallBack::queueIfNotInMainThread(cb,caps->clone()) ) \
	return;

/*!\brief TypeSet of CallBacks with a few extras.  */

mExpClass(Basic) CallBackSet : public ReferencedObject
			     , public TypeSet<CallBack>
{
public:
		CallBackSet();
		CallBackSet(const CallBackSet&);
    CallBackSet& operator=(const CallBackSet&);

    void	doCall(CallBacker*);
		//!< it is possible to remove another callback during the doCall
    void	disableAll(bool yn=true);
    bool	hasAnyDisabled() const;

    void	removeWith(const CallBacker*);
		//!<\note lock lock_ before calling
    void	removeWith(CallBackFunction);
		//!<\note lock lock_ before calling
    void	removeWith(StaticCallBackFunction);
		//!<\note lock lock_ before calling
    void	transferTo(CallBackSet& to,const CallBacker* onlyfor=nullptr,
					   const CallBacker* notfor=nullptr);
		//!<\note lock lock_ before calling, also to's lock_

    mutable Threads::Lock   lock_;

protected:

		~CallBackSet();

};


/*!\brief Inherit from this class to be able to send and/or receive CallBacks */

mExpClass(Basic) CallBacker
{
    friend class	NotifierAccess;

public:
			CallBacker();
			CallBacker(const CallBacker&);
    virtual		~CallBacker();

    CallBacker&		operator =(const CallBacker&) = delete;

    bool		attachCB(const NotifierAccess&,const CallBack&,
				 bool onlyifnew=false) const;
			/*!<Adds cb to notifier, and makes sure
			    it is removed later when object is
			    deleted.
			    \returns if it was attached. */
    bool		attachCB(const NotifierAccess* notif,const CallBack& cb,
				 bool onlyifnew=false) const;
			/*!<\note Attaches only if \a notif is not null.*/

    void		detachCB(const NotifierAccess&,const CallBack&) const;
			/*!<\note Normally not needed if you don't
			          want this explicitly. */
    void		detachCB( const NotifierAccess* notif,
				  const CallBack& cb ) const
			{ if ( notif ) detachCB( *notif, cb ); }
			/*!<\note Detaches only if \a notif is not null.*/

    bool		isNotifierAttached(const NotifierAccess*) const;
			//!<Only for debugging purposes, don't use
    virtual bool	isCapsule() const	{ return false; }
    virtual CallBacker* trueCaller()		{ return this; }

    void		detachAllNotifiers() const;
			//!<Call from the destructor of your inherited object
private:

    bool		notifyShutdown(const NotifierAccess*,bool wait) const;
			/*!<\returns false only if wait and no lock could be
				     obtained. */

    ObjectSet<NotifierAccess>		attachednotifiers_;
    mutable Threads::Lock		attachednotifierslock_;

public:

    void				stopReceivingNotifications() const
					{ detachAllNotifiers(); }

    static void				createReceiverForCurrentThread();
					/*!<Must be called if you wish to send
					   callbacks to this thread. */
    static void				removeReceiverForCurrentThread();
					/*!<Call from your thread before it
					    closes if you have called
					    createReceiverForCurrentThread()
					    in the thread. */

};


/*!\brief Capsule class to wrap any class into a CallBacker.

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

    CBCapsule<T>*	clone() const
			{ return new CBCapsule<T>(data,caller); }
    bool		isCapsule() const override	{ return true; }
    CallBacker*		trueCaller() override		{ return caller; }

};


/*!\ingroup Basic \brief Unpacking data from capsule.

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

#define mCBCapsuleGet(PayLoadType,var,cb) \
CBCapsule<PayLoadType>* var = dynamic_cast< CBCapsule<PayLoadType>* >( cb );

#define mCBCapsuleUnpack(PayLoadType,var,cb) \
mCBCapsuleGet(PayLoadType,cb##caps,cb) \
PayLoadType var = cb##caps->data

#define mCBCapsuleUnpackWithCaller(PayLoadType,var,cber,cb) \
mCBCapsuleGet(PayLoadType,cb##caps,cb) \
PayLoadType var = cb##caps->data; \
CallBacker* cber = cb##caps->caller

#include "notify.h"
