#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "iopar.h"
#include "keyenum.h"
#include "bufstringset.h"
#include "uistring.h"

mFDQtclass(QKeyEvent)


class uiShortcutsMgr;
mGlobal(uiBase) uiShortcutsMgr& SCMgr();
//!< This is where you get your shortcuts


mExpClass(uiBase) uiKeyDesc
{
public:
			uiKeyDesc(const char* statestr=0,const char* keystr=0);
    virtual		~uiKeyDesc();

    bool		operator==(const uiKeyDesc& ev) const
			{ return key_==ev.key_ && state_==ev.state_; }

    bool		set(const char* statestr,const char* keystr);
    BufferString	getKeySequenceStr() const;
    
    OD::ButtonState	state() const 		{ return state_; }
    void		setState( OD::ButtonState bs )	{ state_ = bs; }

    //virtual: just to have the base class be polymorphic, allows dynamic cast
    virtual int		key() const 		{ return key_; }
    void		setKey( int k )		{ key_ = k; }
    char 		asciiChar() const;
    bool		isSimpleAscii() const;

    static const char**	sKeyKeyStrs();
    const char*		stateStr() const;
    const char*		keyStr() const;

    			uiKeyDesc(mQtclass(QKeyEvent*));

protected:

    int			key_;
    OD::ButtonState	state_;

    void		handleSpecialKey(const char*);

};


mExpClass(uiBase) uiShortcutsList
{
public:

			uiShortcutsList(const uiShortcutsList&);
			~uiShortcutsList();

    uiShortcutsList&	operator =(const uiShortcutsList&);
    bool		write(bool usr=true) const;
    
    ObjectSet<uiKeyDesc>&	keyDescs()		{ return keydescs_; }
    const ObjectSet<uiKeyDesc>& keyDescs() const	{ return keydescs_; }
    BufferStringSet& 		names()			{ return names_; }
    const BufferStringSet&	names() const		{ return names_; }
    const uiKeyDesc*		keyDescOf(const char*) const;
    const char*			nameOf(const uiKeyDesc&) const;
    int				valueOf(const uiKeyDesc&) const;

    void			empty();
    void			fillPar(IOPar&) const;

protected:

    friend class	uiShortcutsMgr;
    			uiShortcutsList(const char* selkey);

    BufferString		selkey_;
    ObjectSet<uiKeyDesc>	keydescs_;
    BufferStringSet		names_;

    bool		getKeyValues(const IOPar&,int,
	    			     BufferString&,BufferString&) const;
    bool		getSCNames(const IOPar&,int,BufferString&) const;
    bool		getSCProperties(const IOPar&,int,
	    				uiString&,int&) const;
};


mExpClass(uiBase) uiShortcutsMgr : public CallBacker
{
public:
			uiShortcutsMgr();
			~uiShortcutsMgr();

    const uiShortcutsList& getList(const char* key) const;
    bool		setList(const uiShortcutsList&,bool usr=true);

    Notifier<uiShortcutsMgr>	shortcutsChanged;

protected:

    BufferStringSet	keys_;
    ObjectSet<uiShortcutsList>	lists_;

    friend class	uiShortcutsList;
    IOPar*		getStored(const char*);
    bool		putStored(const char*,const IOPar&);

};


mExpClass(uiBase) uiExtraIntKeyDesc : public uiKeyDesc
{
public:			
			uiExtraIntKeyDesc(const char* statestr=0,
					  const char* keystr=0,
					  int val=1);
			uiExtraIntKeyDesc(const uiExtraIntKeyDesc&);
			~uiExtraIntKeyDesc();
			
    bool		operator==(const uiExtraIntKeyDesc& ev) const
			{ return key_==ev.key_ && state_==ev.state_ &&
			         val_==ev.val_; }

    bool		set(const char* statestr,const char* keystr,int val);
    void		setIntLabel( const uiString& lbl ) { extralbl_ = lbl; }

    int			getIntValue() const	{ return val_; }
    const uiString&	getLabel() const	{ return extralbl_; }

protected:

    int			val_;
    uiString		extralbl_;
};
