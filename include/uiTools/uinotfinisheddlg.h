#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "callback.h"
#include "uistring.h"

class uiNotFinishedDlg;
class uiParent;
class NotFinishedPrompterData;


/*!Presents a list of items that have not finished, giving the user the
   opportunity to abort the application exit or survey change.
\code
MyClass::MyClass()
{
    NotFinishedPrompter::NFP().promptStopping.notify(
	mCB(this,MyClass,askStopCB) );
}

MyClass::~MyClass()
{
    NotFinishedPrompter::NFP().promptStopping.remove(
	mCB(this,MyClass,askStopCB) );
}

void MyClass::askStopCB( CallBacker* )
{
    if ( still_running )
    {
	BufferString str("Process ", name() );
	NotFinishedPrompter::NFP().addProcess( new NotFinishedPrompter::Data(
	    str.buf(), mCB(this,MyClass,stopCB()), nullptr ) );
    }
}


void MyClass::stopCB( CallBacker* )
{
  // Perform graceful exit of running process
    NotFinishedPrompter::NFP().reportSuccessfulStop()
}

\endcode
*/


mExpClass(uiTools) NotFinishedPrompter : public CallBacker
{ mODTextTranslationClass(NotFinishedPrompter);
public:
    static NotFinishedPrompter& NFP(); //gives instance

    Notifier<NotFinishedPrompter>	promptEnding;
	    /*!<Will trigger from when users should be notified of running
	    *	processes that will be terminated at events like survey change
	    *	or application shutdown.
	    */

    int		queueID() const { return queueid_; }
	    /*!<When OK is pressed (i.e. not cancel), a queue is executed.
	    *	If you want something to be executed, add it to this queue.
	    */

    void	addObject(const uiString& str);
	    /*!<Add a object to the list of running processes. Normally called
	    *	when triggered by promptEnding. Use this version to register a
	    *	process that doesn't need any special cleanup before being
	    *	terminated.'
	    *	    \param str	    Description of the process
	    */
    void	addObject(const uiString& str,const CallBack& stopcb,
			  const void* dataptr );
	    /*!<Add a object to the list if running processes. Normally called
	    *	when triggered by promptEnding.Use this version to register a
	    *	process that needs special cleanup before termination.
	    *		\param str	Description of the process
	    *		\param stopcb	Callback that will stop the process
	    *		\param dataptr	Pointer that can be retrieved during
	    *				stopcb
	    */

    void	addObject(const char* str);
    void	addObject(const char* str,const CallBack& stopcb,
					   const void* dataptr );

    void	removeObject(const uiString&);
    void	removeObject(const char*);

    const void* getCurrentObjectData() const;
	    /*!<\returns the dataptr of the currently active object.
	    *	\note Only valid during a call from a cb given in addProcess()
	    */
    uiParent*	getParent();
	    /*!<\returns a pointer to the dialog, which can be used
	    *	when creating a dialog in a callback given in addProcess.
	    *	\note Only valid during a call from a cb given in addProcess()
	    */

    void	reportSuccessfulStop();
	    /*!<Let the dialog know that the current process was successfully
	    *	stopped. Only valid during a call from a cb given in
	    *	addProcess()
	    */

    bool	doTrigger(uiParent*,bool withcancel,const uiString& actiontype);
	    //!<Invoke the system. Returns false if cancel has been pressed.

		~NotFinishedPrompter();

protected:
    friend class uiNotFinishedDlg;

		NotFinishedPrompter();
    void	closeQueueCB(CallBacker*);

    ObjectSet<NotFinishedPrompterData>		objects_;
    uiNotFinishedDlg*				dlg_		= nullptr;
    int						queueid_;
};
