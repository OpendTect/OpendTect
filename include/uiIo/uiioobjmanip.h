#ifndef uiioobjmanip_h
#define uiioobjmanip_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2003
 RCS:           $Id: uiioobjmanip.h,v 1.17 2010-03-11 11:18:00 cvsbert Exp $
________________________________________________________________________

-*/

#include "uibuttongroup.h"
class IOObj;
class MultiID;
class IOStream;
class ioPixmap;
class uiListBox;
class Translator;
class uiToolButton;
class IODirEntryList;
class BufferStringSet;


mClass uiManipButGrp : public uiButtonGroup
{
public:
    			uiManipButGrp(uiParent* p)
			    : uiButtonGroup(p,"") { altbutdata.allowNull(); }
			~uiManipButGrp()
			{ deepErase(butdata); deepErase(altbutdata); }

    enum Type		{ FileLocation, Rename, Remove, ReadOnly };

    uiToolButton*	addButton(Type,const CallBack&,const char* tip);
    uiToolButton*	addButton(const char*,const CallBack&,const char*);
    uiToolButton*	addButton(const ioPixmap&,const CallBack&,const char*);
    void		setAlternative(uiToolButton*,const ioPixmap&,
	    				const char*);
    void		useAlternative(uiToolButton*,bool);

protected:

    mStruct ButData
    {
			ButData(uiToolButton*,const ioPixmap&,const char*);
			~ButData();
	uiToolButton*	but;
	ioPixmap*	pm;
	BufferString	tt;
    };

    ObjectSet<ButData>	butdata;
    ObjectSet<ButData>	altbutdata;
};


class uiIOObjManipGroup;


mClass uiIOObjManipGroupSubj : public CallBacker
{
public:
				uiIOObjManipGroupSubj( uiObject* o )
				    : obj_(o), grp_(0)		{}

    virtual const MultiID*	curID() const			= 0;
    virtual const char*		defExt() const			= 0;
    virtual const BufferStringSet& names() const		= 0;

    virtual void		chgsOccurred()			= 0;
    virtual void		relocStart(const char*)		{}

    uiIOObjManipGroup*	grp_;
    uiObject*		obj_;
};


/*! \brief Buttongroup to manipulate an IODirEntryList. */

mClass uiIOObjManipGroup : public uiManipButGrp
{
public:

			uiIOObjManipGroup(uiIOObjManipGroupSubj&,
					  bool havereloc);
			~uiIOObjManipGroup();

    void		selChg();

    void		triggerButton(uiManipButGrp::Type);

protected:

    uiIOObjManipGroupSubj& subj_;

    uiToolButton*	locbut;
    uiToolButton*	robut;
    uiToolButton*	renbut;
    uiToolButton*	rembut;

    IOObj*		gtIOObj() const;
    void		tbPush(CallBacker*);
    void		relocCB(CallBacker*);

    bool		rmEntry(IOObj*,bool,bool);
    bool		renameEntry(IOObj*,Translator*);
    bool		relocEntry(IOObj*,Translator*);
    bool		readonlyEntry(IOObj*,Translator*);
    void		commitChgs(IOObj*);

    bool		doReloc(Translator*,IOStream&,IOStream&);

};


#endif
