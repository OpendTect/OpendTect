#ifndef uishortcutsmgr_h
#define uishortcutsmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          20/01/2006
 RCS:           $Id: uishortcutsmgr.h,v 1.9 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "iopar.h"
#include "enums.h"
#include "keyenum.h"
#include "bufstringset.h"
class QKeyEvent;


class uiShortcutsMgr;
mGlobal uiShortcutsMgr& SCMgr(); //!< This is where you get your shortcuts


mClass uiKeyDesc
{
public:
			uiKeyDesc(const char* statestr=0,const char* keystr=0);
			
    bool		operator==(const uiKeyDesc& ev) const
			{ return key_==ev.key_ && state_==ev.state_; }

    bool		set(const char* statestr,const char* keystr);
    
    OD::ButtonState	state() const 		{ return state_; }
    void		setState( OD::ButtonState bs )	{ state_ = bs; }
    int			key() const 		{ return key_; }
    void		setKey( int k )		{ key_ = k; }
    char 		asciiChar() const;
    bool		isSimpleAscii() const;

    static const char**	sKeyKeyStrs();
    const char*		stateStr() const;
    const char*		keyStr() const;

    			uiKeyDesc(QKeyEvent*);

protected:

    int			key_;
    OD::ButtonState	state_;

    void		handleSpecialKey(const char*);

};


mClass uiShortcutsList
{
public:

    			uiShortcutsList( const uiShortcutsList& scl )
							{ *this = scl; }
    			~uiShortcutsList() 		{ empty(); }
    uiShortcutsList&	operator =(const uiShortcutsList&);
    bool		write(bool usr=true) const;

    void		fillPar(IOPar&) const;
    
    TypeSet<uiKeyDesc>& keyDescs()		{ return keydescs_; }	
    const TypeSet<uiKeyDesc>& keyDescs() const { return keydescs_; }	
    BufferStringSet& 	names()			{ return names_; }
    const BufferStringSet&  names() const	{ return names_; }
    uiKeyDesc		keyDescOf(const char*) const;
    const char*		nameOf(const uiKeyDesc&) const;

    void		empty();

protected:

    friend class	uiShortcutsMgr;
    			uiShortcutsList(const char* selkey);

    BufferString	selkey_;
    TypeSet<uiKeyDesc>	keydescs_;
    BufferStringSet	names_;

    bool		getKeyValues(const IOPar&,int,
	    			     BufferString&,BufferString&) const;
    bool		getSCNames(const IOPar&,int,BufferString&) const;
};


mClass uiShortcutsMgr
{
public:
			uiShortcutsMgr()			{};
    
    const uiShortcutsList& getList(const char* key) const;
    bool		setList(const uiShortcutsList&,bool usr=true);

protected:

    BufferStringSet	keys_;
    ObjectSet<uiShortcutsList>	lists_;

    friend class	uiShortcutsList;
    IOPar*		getStored(const char*);
    bool		putStored(const char*,const IOPar&);

};

#endif
