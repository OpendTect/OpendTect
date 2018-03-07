#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2001
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "dbkey.h"


namespace Attrib
{
    class Desc;
    class DescID;
    class DescSet;
};

namespace Pick { class Set; }
namespace ZDomain { class Info; }

class AttribParamGroup;
class BufferStringSet;
class CtxtIOObj;
class EvalParam;
class uiAttrDescEd;
class uiAttrTypeSel;
class uiGenInput;
class uiListBox;
class uiPushButton;
class uiToolBar;
class uiToolButton;


/*!\brief Editor for Attribute sets.  */

mExpClass(uiAttributes) uiAttribDescSetEd : public uiDialog
{ mODTextTranslationClass(uiAttribDescSetEd);
public:

    typedef Attrib::Desc	Desc;
    typedef Attrib::DescSet	DescSet;

			uiAttribDescSetEd(uiParent*,DescSet&,
				      uiString prefgrp=uiString::empty(),
				      bool attrsneedupdt=false);
			~uiAttribDescSetEd();

    void		setZDomainInfo(const ZDomain::Info&);
    const ZDomain::Info* getZDomainInfo() const;

    DescSet&		getSet()		{ return attrset_; }
    DBKey		curSetID() const;

    Desc*		curDesc() const;
			//!< Use during operation only!
    int			curDescNr() const;
			//!< Use during operation only!
    void		setCurDescNr(int);
			//!< Use during operation only!
    uiAttrDescEd&	activeDescEd();
			//!< Use during operation only!
    bool		is2D() const;

    void		setSelAttr(const char* attrnm,bool isnewset);
    void		setSensitive(bool);

    void		loadDefaultAttrSet(const char* attribsetnm);

    bool		getUiAttribParamGrps(
				uiParent*,ObjectSet<AttribParamGroup>&,
				BufferStringSet& paramnms,
				TypeSet<BufferStringSet>& usernms);
			//!<Get curDesc() parameter grps and param-usernms info!

    Notifier<uiAttribDescSetEd>		dirshowcb;
    Notifier<uiAttribDescSetEd>		evalattrcb;
    Notifier<uiAttribDescSetEd>		crossevalattrcb;
    Notifier<uiAttribDescSetEd>		xplotcb;
    Notifier<uiAttribDescSetEd>		applycb;

protected:

    DescSet&			attrset_;
    DescSet*			orgattrset_;
    Desc*			prevdesc_;
    ObjectSet<uiAttrDescEd>	desceds_;
    ObjectSet<Desc>		attrdescs_;
    BufferStringSet&		userattrnames_;
    bool			updating_fields_;
    bool			attrsneedupdt_;
    ZDomain::Info*		zdomaininfo_;

    uiToolBar*			toolbar_;
    uiListBox*			attrlistfld_;
    uiAttrTypeSel*		attrtypefld_;

    uiPushButton*		addbut_;
    uiToolButton*		dispbut_;
    uiToolButton*		procbut_;
    uiGenInput*			attrnmfld_;
    uiGenInput*			stornmfld_;
    uiToolButton*		helpbut_;
    uiToolButton*		moveupbut_;
    uiToolButton*		movedownbut_;
    uiToolButton*		sortbut_;
    uiToolButton*		rmbut_;

    void			attrTypSelCB(CallBacker*);
    void			selChgCB(CallBacker*);
    void			savePushCB(CallBacker*);
    void			saveAsPushCB(CallBacker*);
    void			addPushCB(CallBacker*);
    void			rmPushCB(CallBacker*);
    void			moveUpDownCB(CallBacker*);
    void			sortPushCB(CallBacker*);
    void			helpButPushCB(CallBacker*);

    void			autoAttrSetCB(CallBacker*);
    void			newSetCB(CallBacker*);
    void			openSetCB(CallBacker*);
    void			chgAttrInputsCB(CallBacker*);
    void			openDefSetCB(CallBacker*);
    void			importSetCB(CallBacker*);
    void			importFileCB(CallBacker*);
    void			importFromSeisCB(CallBacker*);
    void			job2SetCB(CallBacker*);
    void			crossPlotCB(CallBacker*);
    void			directShowCB(CallBacker*);
    void			procAttributeCB(CallBacker*);
    void			evalAttributeCB(CallBacker*);
    void			crossEvalAttrsCB(CallBacker*);
    void			showMatrixCB(CallBacker*);
    void			graphVizDotPathCB(CallBacker*);
    void			exportToGraphVizDotCB(CallBacker*);

    void			setButStates();
    void			setStorNameFld();
    bool			offerSetSave();
    bool			doSave(bool);
    void			replaceStoredAttr();

    bool			acceptOK();
    bool			rejectOK();

    void			newList(int);
    void			updateFields(bool settype=true);
    bool			doCommit(bool prevdesc=false);
    void			openAttribSet(const DBKey&);
    void			handleFreshSet();
    bool			doAcceptInputs();
    void			handleSensitivity();
    void			updateUserRefs();
    void			ensureValidName(BufferString&) const;
    bool			setUserRef(Desc&);
    Desc*			createAttribDesc(bool checkuref=true);
    void			importFromFile(const char*);
    BufferString		getAttribName(uiAttrDescEd&) const;
    void			getDefaultAttribsets(BufferStringSet& filenms,
						 BufferStringSet& setnms) const;
    void			gtDefAttrSetsInDir(const char* dirnm,
						    BufferStringSet&,
						    BufferStringSet&) const;

    void			createMenuBar();
    void			createToolBar();
    void			createGroups();
    void			init();

public:

    void			updateAllDescsDefaults();
    uiAttrDescEd&		curDescEd()	{ return activeDescEd(); }
    void			updateCurDescEd();

};
