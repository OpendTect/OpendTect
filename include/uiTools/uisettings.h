#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
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
class uiSettingsSubjectTreeItm;
class uiTable;
class uiThemeSel;
class uiTreeView;


mExpClass(uiTools) uiSettingsDlg : public uiDialog
{ mODTextTranslationClass(uiSettingsDlg);
public:

			uiSettingsDlg(uiParent*,const char* grpky=0);
			~uiSettingsDlg();

    bool		hadChanges() const	{ return havechanges_; }
    bool		neededRestart() const	{ return restartneeded_; }
    bool		neededRenewal() const	{ return renewalneeded_; }

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


mExpClass(uiTools) uiSettingsGroup : public uiGroup
{ mODTextTranslationClass(uiSettingsGroup);
public:

    enum Type		{ General, LooknFeel, Interaction };

			mDefineFactory2ParamInClass(uiSettingsGroup,
						    uiParent*,Settings&,
						    factory)
    virtual		~uiSettingsGroup();

    virtual Type	type() const		= 0;
    virtual uiWord	subject() const		= 0;
    virtual const char*	iconID() const		= 0;
    virtual HelpKey	helpKey() const		= 0;

    bool		commit(uiRetVal&);
    void		rollBack()		{ doRollBack(); }

    bool		isChanged() const	{ return changed_; }
    bool		needsRestart() const	{ return needsrestart_; }
    bool		needsRenewal() const	{ return needsrenewal_; }

    static uiString	dispStr(Type);

protected:

			uiSettingsGroup(uiParent*,Settings&);

    void		updateSettings(bool oldval,bool newval,const char* key);
    void		updateSettings(int oldval,int newval,const char* key);
    void		updateSettings(float oldval,float newval,
				       const char* key);
    void		updateSettings(const OD::String& oldval,
				       const OD::String& newval,
				       const char* key);

    virtual void	activationChange(bool activated)	{}
    virtual void	doCommit(uiRetVal&)			= 0;
    virtual void	doRollBack()				{}

    uiRetVal		state_;
    Settings&		setts_;
    bool		changed_;
    bool		needsrestart_;
    bool		needsrenewal_;

public:

			// use if you make your own dialog for user settings
    void		setActive( bool yn )	{ activationChange(yn); }

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
				   tr("Visualization"),
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
