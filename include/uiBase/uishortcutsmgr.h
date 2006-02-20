#ifndef uihandleshortcuts_h
#define uihandleshortcuts_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          20/01/2006
 RCS:           $Id: uishortcutsmgr.h,v 1.3 2006-02-20 18:49:48 cvsbert Exp $
________________________________________________________________________

-*/

#include "iopar.h"
#include "enums.h"

class QKeyEvent;
class EventKeyAndState;

class uiHandleShortcuts 
{
public:
			uiHandleShortcuts(){};

    enum		SCLabels		
    			{ mvForwd, mvBackwd };
    			DeclareEnumUtils(SCLabels);
   

    static bool		handleEvent(QKeyEvent*,SCLabels&);
    static int 		getShortcutIdx(QKeyEvent*);

    static const char*  nameStr()                       { return "Name"; }
    static const char*  keyStr()                        { return "Keys"; }

    static const char*	sKeyFileType;

protected:
};


class ShortcutsList
{
public:
    			ShortcutsList();
    			~ShortcutsList() 		{ deepErase(list_); }

    void		init();
    
    IOPar		readShorcutsFile(bool&);
    bool		getKeyValues(const IOPar&,int,bool,
	    			     BufferString&,BufferString&);
    
    ObjectSet<EventKeyAndState>& getList()		{ return list_; }	

protected:
				
    ObjectSet<EventKeyAndState>	list_;
};

ShortcutsList& SCList();


class EventKeyAndState
{
public:
			EventKeyAndState(QKeyEvent*);
			EventKeyAndState(const char*,const char*);
			
    bool		operator==(const EventKeyAndState& ev) const
			{ return key_==ev.key_ && state_==ev.state_; }
    int			state() const 		{ return state_; }
    void		setState( int state )	{ state_ = state; }

protected:

    int			key_;
    int			state_;

};

#endif
