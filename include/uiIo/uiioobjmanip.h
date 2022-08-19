#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uibuttongroup.h"

class IOObj;
class IOStream;
class Translator;
class uiToolButton;
class IODirEntryList;
class BufferStringSet;


mExpClass(uiIo) uiManipButGrp : public uiButtonGroup
{ mODTextTranslationClass(uiManipButGrp);
public:
			uiManipButGrp(uiParent* p)
			    : uiButtonGroup(p,"ManipButtons",OD::Vertical)
			{ altbutdata.allowNull(); }
			~uiManipButGrp()
			{ deepErase(butdata); deepErase(altbutdata); }

    enum Type		{ FileLocation, Rename, Remove, ReadOnly };

    uiToolButton*	addButton(Type,const uiString& ttip,const CallBack&);
    uiToolButton*	addButton(const char* iconfnm,const uiString& ttip,
				  const CallBack&);
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
				    : obj_(o), grp_(0)		{}

    virtual MultiID		currentID() const			= 0;
    virtual void		getChosenIDs(TypeSet<MultiID>&) const	= 0;
    virtual void		getChosenNames(BufferStringSet&) const	= 0;
    virtual const char*		defExt() const				= 0;
    virtual const BufferStringSet names() const			= 0;

    virtual void		chgsOccurred()				= 0;
    virtual void		relocStart(const char*)			{}

    uiIOObjManipGroup*		grp_;
    uiObject*			obj_;
};


/*! \brief Buttongroup to manipulate an IODirEntryList. */

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
};
