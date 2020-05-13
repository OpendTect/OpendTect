#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2004
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "uigroup.h"
#include "uidialog.h"
#include "uistrings.h"
#include "factory.h"
#include "od_helpids.h"

class Settings;
class uiCheckList;
class uiGenInput;
class uiIconSetSel;
class uiLabel;
class uiMainWin;
class uiSettingsSubjectTreeItm;
class uiTable;
class uiThemeSel;
class uiTreeView;
class uiSettingsGroup;


mExpClass(uiTools) uiSettingsMgr : public CallBacker
{ mODTextTranslationClass(uiSettingsMgr);
public:
		uiSettingsMgr();
		~uiSettingsMgr();

    void	loadToolBarCmds(uiMainWin&);
    void	updateUserCmdToolBar();

    Notifier<uiSettingsMgr> terminalRequested;

private:

    void	keyPressedCB(CallBacker*);
    void	doToolBarCmdCB(CallBacker*);

    BufferStringSet	commands_;
    TypeSet<int>	toolbarids_;

    uiToolBar*	usercmdtb_ = nullptr;

};


mGlobal(uiTools) uiSettingsMgr& uiSettsMgr();


mExpClass(uiTools) uiSettingsDlg : public uiDialog
{ mODTextTranslationClass(uiSettingsDlg);
public:

			uiSettingsDlg(uiParent*,const char* grpky=0);
			~uiSettingsDlg();

    bool		hadChanges() const	{ return havechanges_; }
    bool		neededRestart() const	{ return restartneeded_; }
    bool		neededRenewal() const	{ return renewalneeded_; }

    mDeclInstanceCreatedNotifierAccess(uiSettingsDlg);

    uiSettingsGroup*	getGroup(const char* factky);

protected:

    void		handleRestart();
    bool		rejectOK();
    bool		acceptOK();

    void		initWin(CallBacker*);
    void		selChgCB(CallBacker*);

    uiTreeView*				treefld_;
    uiSettingsSubjectTreeItm*		curtreeitm_;
    uiLabel*				grplbl_;
    ObjectSet<uiSettingsSubjectTreeItm> treeitms_;

    Settings&		setts_;
    bool		havechanges_;
    bool		restartneeded_;
    bool		renewalneeded_;

};


mExpClass(uiTools) uiAdvSettings : public uiDialog
{ mODTextTranslationClass(uiAdvSettings);
public:
			uiAdvSettings(uiParent*,const uiString& titl,
				   const char* settskey=0);
    virtual		~uiAdvSettings();

			// Specify this to edit the survey defaults
    static const char*	sKeySurveyDefs()	{ return "SurvDefs"; }

    static uiDialog*	getPythonDlg(uiParent*);

protected:

    bool		issurvdefs_;
    const IOPar*	cursetts_;
    const IOPar		sipars_;
    ObjectSet<IOPar>	chgdsetts_;

    uiGenInput*		grpfld_;
    uiTable*		tbl_;

    void		setCurSetts();
    void		getChanges();
    bool		commitSetts(const IOPar&);

    const IOPar&	orgPar() const;
    int			getChgdSettIdx(const char*) const;
    void		grpChg(CallBacker*);
    void		dispNewGrp(CallBacker*);
    bool		acceptOK();

};


mExpClass(uiTools) uiSettsGrp : public uiGroup
{
public:

    virtual		~uiSettsGrp()	{}

    virtual void	activationChange(bool)	{}
    virtual bool	commit(uiRetVal&)	= 0;
    virtual void	rollBack()		{}

    bool		isChanged() const	{ return changed_; }
    bool		needsRestart() const	{ return needsrestart_; }
    bool		needsRenewal() const	{ return needsrenewal_; }
    Settings&		settings()		{ return setts_; }

protected:

			uiSettsGrp(uiParent*,Settings&,const char*);

    Settings&		setts_;
    bool		changed_				= false;
    bool		needsrestart_				= false;
    bool		needsrenewal_				= false;

    void		updateSettings(bool oldval,bool newval,const char* key);
    void		updateSettings(int oldval,int newval,const char* key);
    void		updateSettings(float oldval,float newval,
				       const char* key);
    void		updateSettings(const OD::String& oldval,
				       const OD::String& newval,
				       const char* key);

public:

			// Usually called for you
    void		setActive( bool yn )	{ activationChange(yn); }

};


mExpClass(uiTools) uiSettingsSubGroup : public uiSettsGrp
{
public:
			uiSettingsSubGroup(uiSettingsGroup&);

protected:

    void		addToParent(uiSettingsGroup&);
    friend class	uiSettingsGroup;

};


mExpClass(uiTools) uiSettingsGroup : public uiSettsGrp
{ mODTextTranslationClass(uiSettingsGroup);
public:

    enum Type		{ General, LooknFeel, Interaction };

			mDefineFactory2ParamInClass(uiSettingsGroup,
						    uiParent*,Settings&,
						    factory)

    virtual Type	type() const		= 0;
    virtual uiWord	subject() const		= 0;
    virtual const char*	iconID() const		= 0;
    virtual HelpKey	helpKey() const		= 0;

    virtual bool	commit(uiRetVal&);
    virtual void	rollBack();

    static uiString	dispStr(Type);

    uiGroup*		lastGroup();

protected:

			uiSettingsGroup(uiParent*,Settings&);

    virtual void	activationChange(bool)	{}
    virtual void	doCommit(uiRetVal&)	= 0;
    virtual void	doRollBack()		{}

    uiRetVal		state_;
    ObjectSet<uiSettingsSubGroup> subgrps_;
    uiGroup*		bottomobj_		= 0;

    friend class	uiSettingsSubGroup;
    void		add(uiSettingsSubGroup*);

};


#define mDecluiSettingsGroupPublicFns(clss,typ,keystr,icid,usrstr,helpky) \
    mDefaultFactoryInstantiation2Param( uiSettingsGroup, clss, uiParent*, \
					Settings&, keystr, usrstr ); \
 \
    virtual Type	type() const	{ return uiSettingsGroup::typ; } \
    virtual uiWord	subject() const	{ return factoryDisplayName(); } \
    virtual const char*	iconID() const	{ return icid; } \
    virtual HelpKey	helpKey() const	{ return helpky; }



mExpClass(uiTools) uiStorageSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiStorageSettingsGroup);
public:

    mDecluiSettingsGroupPublicFns( uiStorageSettingsGroup,
				   General, "Storage", "database",
				   uiStrings::sStorage(),
				   mTODOHelpKey )

			uiStorageSettingsGroup(uiParent*,Settings&);

protected:

    uiGenInput*		enablesharedstorfld_;

    bool		initialshstorenab_;

    virtual void	doCommit(uiRetVal&);

};


mExpClass(uiTools) uiGeneralLnFSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiGeneralLnFSettingsGroup);
public:

    mDecluiSettingsGroupPublicFns( uiGeneralLnFSettingsGroup,
				   LooknFeel, "GenLnF", "settings",
				   uiStrings::sGeneral(),
				   mODHelpKey(mLooknFeelSettingsHelpID) )

			uiGeneralLnFSettingsGroup(uiParent*,Settings&);

protected:

    uiThemeSel*		themesel_;
    uiIconSetSel*	iconsetsel_;
    uiGenInput*		tbszfld_;

    int			initialtbsz_;

    virtual void	doCommit(uiRetVal&);
    virtual void	doRollBack();

};


mExpClass(uiTools) uiProgressSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiGeneralLnFSettingsGroup);
public:

    mDecluiSettingsGroupPublicFns( uiProgressSettingsGroup,
				   Interaction, "GenInteraction", "progress",
				   uiStrings::sProgress(), mTODOHelpKey )

			uiProgressSettingsGroup(uiParent*,Settings&);

protected:

    uiCheckList*	showprogressfld_;

    bool		initialshowinlprogress_;
    bool		initialshowcrlprogress_;
    bool		initialshowrdlprogress_;

    virtual void	doCommit(uiRetVal&);

};


mExpClass(uiTools) uiVisSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiVisSettingsGroup);
public:

    mDecluiSettingsGroupPublicFns( uiVisSettingsGroup,
				   LooknFeel, "Visualization", "vis",
				   uiStrings::sVisualization(),
				   mTODOHelpKey )

			uiVisSettingsGroup(uiParent*,Settings&);

protected:

    void		mipmappingToggled(CallBacker*);

    uiGenInput*		textureresfactorfld_;
    uiGenInput*		usesurfshadersfld_;
    uiGenInput*		usevolshadersfld_;
    uiGenInput*		enablemipmappingfld_;
    uiGenInput*		anisotropicpowerfld_;
    uiGenInput*		surfdefresfld_;

			//0=standard, 1=higher, 2=highest, 3=system default
    int			initialdefsurfres_;
    int			initialtextureresindex_;
    bool		initialusesurfshaders_;
    bool		initialusevolshaders_;
    bool		initialenablemipmapping_;
    int			initialanisotropicpower_;

    virtual void	doCommit(uiRetVal&);

};
