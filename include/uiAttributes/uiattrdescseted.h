#ifndef uiattrdescseted_h
#define uiattrdescseted_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiattrdescseted.h,v 1.3 2005-08-22 15:33:53 cvsnanne Exp $
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
class uiGenInput;
class uiLabeledComboBox;
class uiLineEdit;
class uiListBox;
class uiPushButton;
class uiToolBar;
class BufferStringSet;
class CtxtIOObj;


/*! \brief Editor for Attribute sets */

class uiAttribDescSetEd : public uiDialog
{
public:

			uiAttribDescSetEd(uiParent*,Attrib::DescSetMan* adsm=0);
			~uiAttribDescSetEd();

    Attrib::DescSet*	getSet()		{ return attrset; }
    const MultiID&	curSetID() const	{ return setid; }

    uiAttrDescEd*	curDescEd();
    			//!< Use during operation only!
    Attrib::Desc*		curDesc() const;
    			//!< Use during operation only!
    int			curDescNr() const;
    			//!< Use during operation only!
    void		updateCurDescEd();

    Notifier<uiAttribDescSetEd>		dirshowcb;
    Notifier<uiAttribDescSetEd>		evalattrcb;

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

    uiToolBar*		toolbar;
    uiListBox*		attrlistfld;
    uiLabeledComboBox*	attrtypefld;
    uiPushButton*	rmbut;
    uiPushButton*	addbut;
    uiPushButton*	revbut;
    uiLineEdit*		attrnmfld;
    uiGenInput*		attrsetfld;

    void		attrTypSel(CallBacker*);
    void		selChg(CallBacker*);
    void		revPush(CallBacker*);
    void		addPush(CallBacker*);
    void		rmPush(CallBacker*);

    void		newSet(CallBacker*);
    void		openSet(CallBacker*);
    void		savePush(CallBacker*);
    void		changeInput(CallBacker*);
    void		defaultSet(CallBacker*);
    void		importSet(CallBacker*);
    void		importFile(CallBacker*);
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
    const char*		getAttrTypeName(const char*);
    bool		doSetIO(bool);

    void		createMenuBar();
    void		createToolBar();
    void		createGroups();
    void		init();

};


#endif
