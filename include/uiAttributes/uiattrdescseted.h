#ifndef uiattrdescseted_h
#define uiattrdescseted_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiattrdescseted.h,v 1.31 2012-08-03 13:00:47 cvskris Exp $
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "multiid.h"

namespace Attrib
{
    class Desc;
    class DescID;
    class DescSet;
    class DescSetMan;
};

namespace Pick { class Set; }
class AttribParamGroup;
class EvalParam;
class uiAttrDescEd;
class uiAttrTypeSel;
class uiGenInput;
class uiListBox;
class uiPushButton;
class uiToolBar;
class BufferStringSet;
class CtxtIOObj;
class IOObj;
class IOPar;
class uiToolButton;

/*! \brief Editor for Attribute sets */

mClass(uiAttributes) uiAttribDescSetEd : public uiDialog
{
public:

			uiAttribDescSetEd(uiParent*,Attrib::DescSetMan* adsm,
					  const char* prefgrp =0,
					  bool attrsneedupdt =false);
			~uiAttribDescSetEd();

    Attrib::DescSet*	getSet()		{ return attrset_; }
    const MultiID&	curSetID() const	{ return setid_; }

    uiAttrDescEd*	curDescEd();
    			//!< Use during operation only!
    Attrib::Desc*	curDesc() const;
    			//!< Use during operation only!
    int			curDescNr() const;
    			//!< Use during operation only!
    void		updateCurDescEd();
    bool		is2D() const;

    void		setSensitive(bool);

    bool		getUiAttribParamGrps(
	    			uiParent*,ObjectSet<AttribParamGroup>&,
				BufferStringSet& paramnms,
				TypeSet<BufferStringSet>& usernms);
    			//!<Get curDesc() parameter grps and param-usernms info!

    Notifier<uiAttribDescSetEd>		dirshowcb;
    Notifier<uiAttribDescSetEd>		evalattrcb;
    Notifier<uiAttribDescSetEd>		crossevalattrcb;
    Notifier<uiAttribDescSetEd>		xplotcb;

    static const char* 	sKeyUseAutoAttrSet;
    static const char* 	sKeyAuto2DAttrSetID;
    static const char*  sKeyAuto3DAttrSetID;

protected:

    Attrib::DescSetMan*		inoutadsman_;
    Attrib::DescSetMan*		adsman_;
    Attrib::DescSet*		attrset_;
    Attrib::Desc*		prevdesc_;
    MultiID			setid_;
    ObjectSet<uiAttrDescEd> 	desceds_;
    ObjectSet<Attrib::Desc> 	attrdescs_;
    BufferStringSet&		userattrnames_;
    CtxtIOObj&			setctio_;
    MultiID			cancelsetid_;
    bool			updating_fields_;
    bool			attrsneedupdt_;
    static BufferString		nmprefgrp_;

    uiToolBar*			toolbar_;
    uiListBox*			attrlistfld_;
    uiAttrTypeSel*		attrtypefld_;

    uiPushButton*		addbut_;
    uiPushButton*		revbut_;
    uiGenInput*			attrnmfld_;
    uiGenInput*			attrsetfld_;
    uiToolButton*       	helpbut_;
    uiToolButton*       	moveupbut_;
    uiToolButton*       	movedownbut_;
    uiToolButton*       	sortbut_;
    uiToolButton*		rmbut_;

    void			attrTypSel(CallBacker*);
    void			selChg(CallBacker*);
    void			revPush(CallBacker*);
    void			addPush(CallBacker*);
    void			rmPush(CallBacker*);
    void			moveUpDownCB(CallBacker*);
    void                	sortPush(CallBacker*);
    void                	helpButPush(CallBacker*);

    void			autoSet(CallBacker*);
    void			newSet(CallBacker*);
    void			openSet(CallBacker*);
    void                	openAttribSet(const IOObj*);
    void			savePush(CallBacker*);
    void			saveAsPush(CallBacker*);
    void			changeInput(CallBacker*);
    void			defaultSet(CallBacker*);
    void			getDefaultAttribsets(BufferStringSet&,
	    					     BufferStringSet&);
    void			importSet(CallBacker*);
    void			importFile(CallBacker*);
    void			job2Set(CallBacker*);
    void			crossPlot(CallBacker*);
    void			directShow(CallBacker*);
    void			evalAttribute(CallBacker*);
    void			crossEvalAttrs(CallBacker*);
    void			importFromFile(const char*);

    void			setButStates();
    bool			offerSetSave();
    bool			doSave(bool);
    void			replaceStoredAttr();
    void			replaceStoredAttr(IOPar&);
    void			removeNotUsedAttr();
    //bool		hasInput(const Attrib::Desc&,const Attrib::DescID&);

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);

    void			newList(int);
    void			updateFields(bool settype=true);
    bool			doCommit(bool prevdesc=false);
    void			handleSensitivity();
    void			updateUserRefs();
    bool			validName(const char*) const;
    bool			setUserRef(Attrib::Desc*);
    void			updateAttrName();
    bool			doSetIO(bool);
    Attrib::Desc*		createAttribDesc(bool checkuref=true);

    void			createMenuBar();
    void			createToolBar();
    void			createGroups();
    void			init();

public:

    void			updtAllEntries();
};


#endif

