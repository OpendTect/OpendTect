#ifndef undo_h
#define undo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "basicmod.h"
#include "bufstringset.h"
#include "callback.h"

class UndoEvent;
class BinID;

/*!
\ingroup Basic
\brief Class to handle undo/redo information. Events that can be undone/redone
are added to the Undo. One user operation may involve thousands of changes
added to the history, but the user does not want to press undo a thousand times.This is managed by setting a UserInteractionEnd flag on the last event in a
chain that the user started. When doing undo, one undo step is consists of all
events from the current event until the next event with the UserInteraction
flag set. 

  This means that after all user-driven events, the UserInteractionEnd should
  be set:
  \code
  void MyClass::userPushedAButtonCB( CallBacker* )
  {
      doSomethingsThatAddThingsOnTheHistory();
      history.setUserInteractionEnd( currentEventID() );
  }
  \endcode
*/

mClass(Basic) Undo : public CallBacker
{
public:
	    			Undo();
    virtual			~Undo();

    void			removeAll();
    int				maxLength() const;
    				/*!<Returns maximum number of userevents.
				    \note The actual number of events may
				          be considerably higher since many
				 	  events may be part of one single
				 	  user event. */
    void			setMaxLength(int);
    int				addEvent(UndoEvent* event,
	    				 const char* description=0);
    				/*!<\param event The new event (becomes mine).
				    \return the event id. */
    int				currentEventID() const;
    int				firstEventID() const;
    int				lastEventID() const;

    void			removeAllAfterCurrentEvent();
    void			removeAllBeforeCurrentEvent();

    bool			isUserInteractionEnd(int eventid) const;
    void			setUserInteractionEnd(int eventid,bool=true);
    int				getNextUserInteractionEnd(int start) const;

    BufferString		getDesc(int eventid) const;
    void			setDesc(int eventid,const char* d);
    BufferString		unDoDesc() const;
    BufferString		reDoDesc() const;
    				/*!\note takes redodesc with userinteraction */

    bool			canUnDo() const;
    bool			unDo(int nrtimes=1,bool userinteraction=true);

    bool			canReDo() const;
    bool			reDo(int nrtimes=1,bool userinteraction=true);

    Notifier<Undo>		changenotifier; //Any change
    Notifier<Undo>		undoredochange; //can**Do() or **DoDesc() change

protected:
    int				indexOf(int eventid) const;

    void			removeOldEvents();
    void			removeStartToAndIncluding(int);
    int				currenteventid_; 
    int				firsteventid_;
    int				maxsize_;

    ObjectSet<UndoEvent>	events_;
    int				userendscount_;
};


/*!
\ingroup Basic
\brief Holds the information on how to undo/redo something.
*/

mClass(Basic) UndoEvent
{
public:
    				UndoEvent();
    virtual			~UndoEvent();

    BufferString		getDesc() const;
    void			setDesc(const char*);

    void			setUserInteractionEnd(bool=true);
    bool			isUserInteractionEnd() const;

    virtual const char*		getStandardDesc() const	 	= 0;
    virtual bool		unDo()				= 0;
    virtual bool		reDo()				= 0;

protected:
    BufferString*		desc_;
    bool			isuserinteractionend_;
};


/*!
\ingroup Basic
\brief BinID UndoEvent.
*/

mClass(Basic) BinIDUndoEvent : public UndoEvent
{
public:
    virtual const BinID&	getBinID() const;
};

#endif

