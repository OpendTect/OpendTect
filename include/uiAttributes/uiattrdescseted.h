#ifndef uiattrdescseted_h
#define uiattrdescseted_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiattrdescseted.h,v 1.14 2007-12-06 11:07:58 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"

namespace Attrib
{
    class Desc;
    class DescID;
    class DescSet;
    class DescSetMan;
};

class uiAttrDescEd;
class uiAttrTypeSel;
class uiGenInput;
class uiLineEdit;
class uiListBox;
class uiPushButton;
class uiToolBar;
class BufferStringSet;
class CtxtIOObj;
class IOObj;
class uiToolButton;

/*! \brief Editor for Attribute sets */

class uiAttribDescSetEd : public uiDialog
{
public:

			uiAttribDescSetEd(uiParent*,Attrib::DescSetMan* adsm,
					  const char* prefgrp =0);
			~uiAttribDescSetEd();

    Attrib::DescSet*	getSet()		{ return attrset; }
    const MultiID&	curSetID() const	{ return setid; }

    void		autoSet();
    uiAttrDescEd*	curDescEd();
    			//!< Use during operation only!
    Attrib::Desc*		curDesc() const;
    			//!< Use during operation only!
    int			curDescNr() const;
    			//!< Use during operation only!
    void		updateCurDescEd();
    bool		is2D() const;

    void		setSensitive(bool);

    Notifier<uiAttribDescSetEd>		dirshowcb;
    Notifier<uiAttribDescSetEd>		evalattrcb;

    static const char* 	sKeyUseAutoAttrSet;
    static const char* 	sKeyAuto2DAttrSetID;
    static const char*  sKeyAuto3DAttrSetID;

protected:

    Attrib::DescSetMan*	inoutadsman;
    Attrib::DescSetMan*	adsman;
    Attrib::DescSet*	attrset;
    Attrib::Desc*	prevdesc;
    MultiID		setid;
    ObjectSet<uiAttrDescEd> desceds;
    ObjectSet<Attrib::Desc> attrdescs;
    BufferStringSet&	userattrnames;
    CtxtIOObj&		setctio;
    MultiID		cancelsetid;
    bool		updating_fields;
    static BufferString	nmprefgrp;

    uiToolBar*		toolbar;
    uiListBox*		attrlistfld;
    uiAttrTypeSel*	attrtypefld;
    uiPushButton*	rmbut;
    uiPushButton*	addbut;
    uiPushButton*	revbut;
    uiLineEdit*		attrnmfld;
    uiGenInput*		attrsetfld;
    uiToolButton*       helpbut;

    void		attrTypSel(CallBacker*);
    void		selChg(CallBacker*);
    void		revPush(CallBacker*);
    void		addPush(CallBacker*);
    void		rmPush(CallBacker*);
    void                helpButPush(CallBacker*);

    void		newSet(CallBacker*);
    void		openSet(CallBacker*);
    void                openAttribSet(const IOObj*);
    void		savePush(CallBacker*);
    void		changeInput(CallBacker*);
    void		defaultSet(CallBacker*);
    void		getDefaultAttribsets(BufferStringSet&,BufferStringSet&);
    void		importSet(CallBacker*);
    void		importFile(CallBacker*);
    void		job2Set(CallBacker*);
    void		crossPlot(CallBacker*);
    void		directShow(CallBacker*);
    void		evalAttribute(CallBacker*);
    void		importFromFile(const char*);

    bool		offerSetSave();
    bool		doSave(bool);
    void		replaceStoredAttr();
    void		removeNotUsedAttr();
    bool		hasInput(const Attrib::Desc&,const Attrib::DescID&);

    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);

    void		newList(int);
    void		updateFields(bool settype=true);
    bool		doCommit(bool prevdesc=false);
    void		handleSensitivity();
    void		updateUserRefs();
    bool		validName(const char*) const;
    bool		setUserRef(Attrib::Desc*);
    void		updateAttrName();
    bool		doSetIO(bool);
    Attrib::Desc*	createAttribDesc(bool checkuref=true);

    void		createMenuBar();
    void		createToolBar();
    void		createGroups();
    void		init();

};


#endif
