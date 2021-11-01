#pragma once
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
#include "callback.h"


/*!\brief Interface class for Notifier. See comments there.  */

mExpClass(Basic) NotifierAccess
{

    friend class	NotifyStopper;
    friend class	CallBacker;

public:

			NotifierAccess(const NotifierAccess&);
			NotifierAccess();
    virtual		~NotifierAccess();

    inline bool		isEmpty() const		{ return cbs_.isEmpty(); }
    inline void		setEmpty()		{ cbs_.setEmpty(); }
    inline bool		isEnabled() const     { return !cbs_.hasAnyDisabled(); }
    inline void		enable( bool yn=true )	{ cbs_.disableAll(!yn); }
    inline void		disable()		{ cbs_.disableAll(true); }

    bool		willCall(const CallBacker*) const;
			/*!<\returns true if the callback list contains
			     CallBacker. */
    void		notify(const CallBack&,bool first=false) const;
    bool		notifyIfNotNotified(const CallBack&) const;
			//!\returns true if it was added
    void		remove(const CallBack&) const;
    bool		removeWith(const CallBacker*,bool wait=true) const;
			//!<\returns false only if wait and no lock could be got

    void		transferCBSTo(const NotifierAccess&,
				      const CallBacker* only_for,
				      const CallBacker* not_for=0) const;

    CallBackSet&	cbs_;
    CallBacker*		cber_;

protected:

    static void		doTrigger(CallBackSet&,const CallBacker*);
    void		addShutdownSubscription(const CallBacker*) const;
    bool		removeShutdownSubscription(const CallBacker*,
						    bool wait) const;
			//!<\returns false only if wait and no lock could be got
			/*!\returns previous status */

    mutable ObjectSet<const CallBacker>	shutdownsubscribers_;
    mutable Threads::Lock	shutdownsubscriberlock_;

public:

    bool		isShutdownSubscribed(const CallBacker*) const;
			//!<Only for debugging purposes, don't use

};


/*!\brief Class to help setup a callback handling.

  The two things:
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

  Note that the Notifier system circumvents the const system. Why? This makes
  gettting and sending callbacks to/from const objects possible without
  difficult const_casts.

*/


template <class T>
mClass(Basic) Notifier : public NotifierAccess
{
public:

			// Following functions are usually used by T class only:
			Notifier( T* c )			{ cber_ = c; }

    inline void		trigger()
			{
			    if ( !cbs_.isEmpty() )
				doTrigger( cbs_, cber_ );
			}
    inline void		trigger( T& t )
			{
			    if ( !cbs_.isEmpty() )
				doTrigger( cbs_, &t );
			}
    inline void		trigger( CallBacker* c )
			{
			    if ( !cbs_.isEmpty() )
				doTrigger( cbs_, c );
			}

};


#define mAttachObjCB( objptr, notifier, func, chk ) \
    attachCB( notifier, CallBack( objptr, mCBFn(func) ), chk )
#define mDetachObjCB( objptr, notifier, func ) \
    detachCB( notifier, CallBack( objptr, mCBFn(func) ) )

#define mAttachCB( notifier, func ) \
    mAttachObjCB( this, notifier, func, false )

#define mAttachCBIfNotAttached( notifier, func ) \
    mAttachObjCB( this, notifier, func, true )

#define mDetachCB( notifier, func ) \
    mDetachObjCB( this, notifier, func )


/*!
\brief Notifier with automatic capsule creation.

  When non-callbacker data needs to be passed, you can put it in a capsule.

  You'll need to define:

  \code
  CNotifier<MyClass,const uiMouseEvent&>	mousepress;
  \endcode
*/

template <class T,class PayLoad>
mClass(Basic) CNotifier : public NotifierAccess
{
public:
			CNotifier( T* cb )	{ cber_ = cb; }

    inline void		trigger( PayLoad pl )
			{
			    if ( !cbs_.isEmpty() )
			    {
				CBCapsule<PayLoad> caps( pl, cber_ );
				doTrigger( cbs_, &caps );
			    }
			}

    inline void		trigger( PayLoad pl, CallBacker* cb )
			{
			    if ( !cb )
				trigger( pl );
			    else if ( !cbs_.isEmpty() )
			    {
				CBCapsule<PayLoad> caps( pl, cb );
				doTrigger( cbs_, &caps );
			    }
			}
    inline void		trigger( PayLoad pl, T& t )
			{
			    trigger(pl,&t);
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

  To stop notifications only to a certain object (usually 'this'), pass the
  object as 2nd argument to the constructor.
*/

mExpClass(Basic) NotifyStopper
{
public:
			NotifyStopper(NotifierAccess&,
				      const CallBacker* only_for=nullptr);
			~NotifyStopper();

    void		enableNotification();
    void		disableNotification();

protected:

    NotifierAccess&	thenotif_;
    bool		isdisabled_;
    const CallBacker*	onlyfor_;

    void		setDisabled(bool);

public:

    mDeprecated("use enableNotification") void restore()
			{ enableNotification(); }
    mDeprecated("use enableNotification") void enable()
			{ enableNotification(); }
    mDeprecated("use disableNotification") void disable()
			{ disableNotification(); }

};


//! Set of macros to add an instanceCreated() notifier
//! This can provide a notification of any instance of a class being produced

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
