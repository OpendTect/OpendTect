#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2003
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uibuttongroup.h"
#include "dbkey.h"

class IOObj;
class IOStream;
class Translator;
class uiToolButton;
class BufferStringSet;


mExpClass(uiIo) uiManipButGrp : public uiButtonGroup
{ mODTextTranslationClass(uiManipButGrp);
public:
			uiManipButGrp(uiParent* p)
			    : uiButtonGroup(p,"ManipButtons",OD::Vertical)
			{ altbutdata.setNullAllowed(); }
			~uiManipButGrp()
			{ deepErase(butdata); deepErase(altbutdata); }

    enum Type		{ FileLocation, Rename, Remove, ReadOnly };

    uiToolButton*	addButton(Type,const uiString& ttip,const CallBack&);
    uiToolButton*	addButton(const char* iconfnm,const uiString& ttip,
				  const CallBack&);
    void		remove(uiToolButton*);
    void		setAlternative(uiToolButton*,const char* icfnm,
				       const uiString& ttip);
    void		useAlternative(uiToolButton*,bool);

protected:

    mStruct(uiIo) ButData
    {
			ButData(uiToolButton*,const char*,const uiString&);
	uiToolButton*	but;
	BufferString	pmnm;
	uiString	tt;
    };

    ObjectSet<ButData>	butdata;
    ObjectSet<ButData>	altbutdata;
};


class uiIOObjManipGroup;


mExpClass(uiIo) uiIOObjManipGroupSubj : public CallBacker
{ mODTextTranslationClass(uiIOObjManipGroupSubj);
public:
				uiIOObjManipGroupSubj( uiObject* o )
				    : obj_(o), grp_(0)			{}
				~uiIOObjManipGroupSubj();

    virtual DBKey		currentID() const			= 0;
    virtual void		getChosenIDs(DBKeySet&) const	= 0;
    virtual void		getChosenNames(BufferStringSet&) const	= 0;
    virtual const char*		defExt() const				= 0;
    virtual const BufferStringSet& names() const			= 0;

    virtual void		chgsOccurred()				= 0;
    virtual void		relocStart(const char*)			{}

    uiIOObjManipGroup*		grp_;
    uiObject*			obj_;
};


/*! \brief Buttongroup to manipulate entries in DBDir. */

mExpClass(uiIo) uiIOObjManipGroup : public uiManipButGrp
{ mODTextTranslationClass(uiIOObjManipGroup);
public:
			uiIOObjManipGroup(uiIOObjManipGroupSubj&,
					  bool havereloc,
					  bool haveremove=true);
			~uiIOObjManipGroup();

    void		selChg();

    void		triggerButton(uiManipButGrp::Type);

protected:

    uiIOObjManipGroupSubj& subj_;

    uiToolButton*	locbut;
    uiToolButton*	robut;
    uiToolButton*	renbut;
    uiToolButton*	rembut;


    void		tbPush(CallBacker*);
    void		relocCB(CallBacker*);

    bool		rmEntry(IOObj&);
    bool		rmEntries(ObjectSet<IOObj>&);
    bool		renameEntry(IOObj&,Translator*);
    bool		relocEntry(IOObj&,Translator*);
    bool		readonlyEntry(IOObj&, Translator*,bool set2ro);

    bool		doReloc(Translator*,IOStream&,IOStream&);

};
