#ifndef uihandleshortcuts_h
#define uihandleshortcuts_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          20/01/2006
 RCS:           $Id: uishortcutsmgr.h,v 1.1 2006-01-31 16:50:23 cvshelene Exp $
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

protected:
};


class ShortcutsList
{
public:
    			ShortcutsList();
    			~ShortcutsList() 		{ deepErase(list_); }

    void		init();
    
    IOPar		readShorcutsFile();
    bool		getKeyValues(const IOPar&,int,BufferString&,
	    			     BufferString&);
    
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

protected:

    int			key_;
    int			state_;

};

#endif
