#ifndef uishortcutsmgr_h
#define uishortcutsmgr_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          20/01/2006
 RCS:           $Id: uishortcutsmgr.h,v 1.5 2006-05-08 07:06:31 cvshelene Exp $
________________________________________________________________________

-*/

#include "iopar.h"
#include "enums.h"
#include "bufstringset.h"


class uiShortcutsList;
class uiShortcutsMgr
{
public:
			uiShortcutsMgr()			{};

    IOPar*		readShortcutsFile(const char*);
    			// IOPar becomes yours!
    
    uiShortcutsList*	getList(const char*);
};

uiShortcutsMgr& SCMgr();



class QKeyEvent;
class uiKeyDesc;

class uiShortcutsList
{
public:
    			uiShortcutsList(const char*);
    			~uiShortcutsList() 		{ deepErase(keyslist_);}

    void		init(const char*);
    
    bool		getKeyValues(const IOPar&,int,
	    			     BufferString&,BufferString&);
    bool		getSCNames(const IOPar&,int,BufferString&);
    const char*		getAct(QKeyEvent*);		
    
    ObjectSet<uiKeyDesc>& getKeysList()			{ return keyslist_; }	
    BufferStringSet& 	getNamesList()			{ return nameslist_; }

    static const char*  ODSceneStr()			{ return "ODScene"; }   

protected:
				
    ObjectSet<uiKeyDesc> keyslist_;
    BufferStringSet	nameslist_;
};


class uiKeyDesc
{
public:
			uiKeyDesc(QKeyEvent*);
			~uiKeyDesc();
			uiKeyDesc(const char*,const char*);
			
    bool		operator==(const uiKeyDesc& ev) const
			{ return key_==ev.key_ && state_==ev.state_; }

    bool		set( const char* str1, const char* str2 );
    bool		set(QKeyEvent*);
    
    int			state() const 		{ return state_; }
    int			key() const 		{ return key_; }
    QKeyEvent*		keyEv() const		{ return qkeyev_; }
    void		setState(int);
    char 		asciiChar() const;
    bool		isSimpleAscii() const;

protected:

    int			key_;
    int			state_;
    QKeyEvent*		qkeyev_;
    bool		qkeyevmine_;

    void		handleSpecialKey(const char*);

};

#endif
