#ifndef notify_h
#define notify_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert / Kris
 Date:		Nov 1995 / April 2016
 Contents:	Notifications
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

    void		notify(const CallBack&,bool first=false);
    bool		notifyIfNotNotified(const CallBack&);
			//!\returns true if it was added
    void		remove(const CallBack&);
    bool		removeWith(CallBacker*,bool wait=true);
			//!<\returns false only if wait and no lock could be got

    inline bool	isEnabled() const	{ return cbs_.isEnabled(); }
    inline bool	enable( bool yn=true )	{ return cbs_.doEnable(yn); }
    inline bool	disable()		{ return cbs_.doEnable(false); }

    inline bool	isEmpty() const	{ return cbs_.isEmpty(); }
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


#define mAttachCB( notifier, func ) \
attachCB( notifier, CallBack( this, ((CallBackFunction)(&func) ) ), false )

#define mAttachCBIfNotAttached( notifier, func ) \
attachCB( notifier, CallBack( this, ((CallBackFunction)(&func) ) ), true )

#define mDetachCB( notifier, func ) \
detachCB( notifier, CallBack( this, ((CallBackFunction)(&func) ) ) )


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


#endif
