#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
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
    class DescSetMan;
};

namespace Pick { class Set; }
namespace ZDomain { class Info; }

class AttribParamGroup;
class BufferStringSet;
class CtxtIOObj;
class EvalParam;
class IOObj;
class uiAttrDescEd;
class uiAttrTypeSel;
class uiGenInput;
class uiListBox;
class uiPushButton;
class uiToolBar;
class uiToolButton;


/*!
\brief Editor for Attribute sets.
*/

mExpClass(uiAttributes) uiAttribDescSetEd : public uiDialog
{ mODTextTranslationClass(uiAttribDescSetEd);
public:

			uiAttribDescSetEd(uiParent*,Attrib::DescSetMan* adsm,
					  const char* prefgrp =0,
					  bool attrsneedupdt =false);
			~uiAttribDescSetEd();

    void		setZDomainInfo(const ZDomain::Info&);
    const ZDomain::Info* getZDomainInfo() const;

    void		setDescSetMan(Attrib::DescSetMan*);
    Attrib::DescSet*	getSet()		{ return attrset_; }
    const DBKey&	curSetID() const	{ return setid_; }

    Attrib::Desc*	curDesc() const;
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

    static const char*	sKeyUseAutoAttrSet;
    static const char*	sKeyAuto2DAttrSetID;
    static const char*	sKeyAuto3DAttrSetID;

protected:

    Attrib::DescSetMan*		inoutadsman_;
    Attrib::DescSetMan*		adsman_;
    Attrib::DescSet*		attrset_;
    Attrib::Desc*		prevdesc_;
    DBKey			setid_;
    ObjectSet<uiAttrDescEd>	desceds_;
    ObjectSet<Attrib::Desc>	attrdescs_;
    BufferStringSet&		userattrnames_;
    CtxtIOObj&			setctio_;
    DBKey			cancelsetid_;
    bool			updating_fields_;
    bool			attrsneedupdt_;
    static BufferString		nmprefgrp_;
    ZDomain::Info*		zdomaininfo_;

    uiToolBar*			toolbar_;
    uiListBox*			attrlistfld_;
    uiAttrTypeSel*		attrtypefld_;

    uiPushButton*		addbut_;
    uiToolButton*		dispbut_;
    uiToolButton*		procbut_;
    uiGenInput*			attrnmfld_;
    uiGenInput*			attrsetfld_;
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
    bool			offerSetSave();
    bool			doSave(bool);
    void			replaceStoredAttr();
    void			replaceStoredAttr(IOPar&);
    void			removeUnusedAttrDescs();

    bool			acceptOK();
    bool			rejectOK();

    void			newList(int);
    void			updateFields(bool settype=true);
    bool			doCommit(bool prevdesc=false);
    void			openAttribSet(const IOObj*);
    bool			doAcceptInputs();
    void			handleSensitivity();
    void			updateUserRefs();
    void			ensureValidName(BufferString&) const;
    bool			setUserRef(Attrib::Desc&);
    bool			doSetIO(bool);
    Attrib::Desc*		createAttribDesc(bool checkuref=true);
    void			importFromFile(const char*);
    BufferString		getAttribName(uiAttrDescEd&) const;
    void			getDefaultAttribsets(BufferStringSet&,
						     BufferStringSet&,
						     BufferStringSet&);

    void			createMenuBar();
    void			createToolBar();
    void			createGroups();
    void			init();

public:

    void			updateAllDescsDefaults();
    uiAttrDescEd&		curDescEd()	{ return activeDescEd(); }
    void			updateCurDescEd();

};
