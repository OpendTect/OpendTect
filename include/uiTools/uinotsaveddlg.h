#ifndef uinotsaveddlg_h
#define uinotsaveddlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2010
 RCS:           $Id: uinotsaveddlg.h,v 1.4 2012-08-03 13:01:14 cvskris Exp $
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "bufstring.h"
#include "callback.h"

class uiNotSavedDlg;
class uiParent;
class NotSavedPrompterData;


/*!Presents a list of items that have not been saved, giving the user the
   opportunity to save them.
\code
MyClass::MyClass()
{
    NotSavedPrompter::NSP().promptSaving.notify(
        mCB(this,MyClass,askSaveCB) );
}

MyClass::~MyClass()
{
    NotSavedPrompter::NSP().promptSaving.remove(
        mCB(this,MyClass,askSaveCB) );
}

void MyClass::askSaveCB( CallBacker* )
{
    if ( not_saved )
    {
        BufferString str("Object type \"" );
	str += name(); str += "\"";

	PtrMan<IOObj> ioobj = IOM().get( mid );
	const bool issaveas = (bool) ioobj;

        NotSavedPrompter::NSP().addObject( new NotSavedPrompter::Data(
	    str.buf(), mCB(this,MyClass,saveCB()), issaveas, 0 ) );
    }
}


void MyClass::saveCB( CallBacker* )
{
    if ( NotSavedPrompter::NSP().isSaveAs() )
    {
        uiParent* uip = NotSavedPrompter::NSP().getParent();
	//Start dlg with uiparent and get new name
    }

    if ( successfulSave )
        NotSavedPrompter::NSP().reportSuccessfullSave()
}

\endcode
*/

   
mClass(uiTools) NotSavedPrompter : public CallBacker
{
public:
    static NotSavedPrompter&	NSP(); //gives instance

    Notifier<NotSavedPrompter>	promptSaving;
    				/*!<Will trigger from when users should save all
				    unsaved objects, normally at survey change
				    or shutdown. */
    int		queueID() const { return queueid_; }
    		/*!<When OK is pressed (i.e. not cancel), a queue is executed.
		    If you want something to be executed, add it to this queue.
	        */ 
		    
    void	addObject(const char* str,const CallBack& savecb,bool issaveas,
			  const void* dataptr );
    		/*!<Lets the object know that you have an object that should
		    be added to the list of unsaved objects. Normally called
		    when triggered by promptSaving.
		    \param str	    Description of the object (e.g. "Horizon A")
		    \param savecb   Callback that will save the object
		    \param issaveas true if savecb will prompt user for a name
		    \param dataptr  Pointer that can be retrieved during savecb
		*/
				
    bool 	isSaveAs() const;
		/*!<\returns the issaveas status of the currently active object
		     \note Only valid during a call from a cb
			   given in addObject() */
    const void*	getCurrentObjectData() const;
    		/*!<\returns the dataptr of the currently active object.
		    \note Only valid during a call from a cb given in
			  addObject() */
    uiParent*	getParent();
		/*!<\returns a pointer to the save-dialog, which can be used
		       when creating a dialog in a callback given in addObject.
		   \note Only valid during a call from a cb given in
			  addObject() */

    void	reportSuccessfullSave();
    		/*!<Let the dialog know that the current object was successfully
		    saved.
		   \note Only valid during a call from a cb given in
			  addObject() */
    		
		NotSavedPrompter();
    bool	doTrigger(uiParent*,bool withcancel,const char* actiontype);
    		//!<Invoke the system. Returns false if cancel has been pressed.

protected:
    friend class uiNotSavedDlg;

    ObjectSet<NotSavedPrompterData>		objects_;
    uiNotSavedDlg*				dlg_;
    int						queueid_;
};


#endif

