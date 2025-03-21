#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uidlggroup.h"
#include "factory.h"
#include "uistring.h"

class Settings;
class uiComboBox;
class uiGenInput;
class uiLabeledComboBox;
class uiMaterialGroup;
class uiTable;


mExpClass(uiTools) uiSettings : public uiDialog
{ mODTextTranslationClass(uiSettings);
public:
			uiSettings(uiParent*,const char* titl,
				   const char* settskey=0);
    virtual		~uiSettings();

			// Specify this to edit the survey defaults
    static const char*	sKeySurveyDefs()	{ return "SurvDefs"; }

    static uiDialog*	getPythonDlg(uiParent*,const char* titl);

protected:

    bool		issurvdefs_;
    const IOPar*	cursetts_;
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
    bool		acceptOK(CallBacker*) override;

};


mExpClass(uiTools) uiSettingsGroup : public uiDlgGroup
{ mODTextTranslationClass(uiSettingsGroup);
public:
			mDefineFactory2ParamInClass(uiSettingsGroup,
						    uiParent*,Settings&,
						    factory)
    virtual		~uiSettingsGroup();

    bool		isChanged() const	{ return changed_; }
    bool		needsRestart() const	{ return needsrestart_; }
    bool		needsRenewal() const	{ return needsrenewal_; }
    const uiString	errMsg() const override;

protected:
			uiSettingsGroup(uiParent*,const uiString& caption,
					Settings&);

    void		updateSettings(bool oldval,bool newval,const char* key);
    void		updateSettings(int oldval,int newval,const char* key);
    void		updateSettings(float oldval,float newval,
				       const char* key);
    void		updateSettings(const OD::String& oldval,
				       const OD::String& newval,
				       const char* key);
    void		updateSettings(const char* oldval,
				       const char* newval,
				       const char* key);

    uiString		errmsg_;
    Settings&		setts_;
    bool		changed_;
    bool		needsrestart_;
    bool		needsrenewal_;
};


mExpClass(uiTools) uiSettingsDlg : public uiTabStackDlg
{ mODTextTranslationClass(uiSettingsDlg);
public:
			uiSettingsDlg(uiParent*);
			~uiSettingsDlg();

    bool		isChanged() const	{ return changed_; }
    bool		needsRestart() const	{ return needsrestart_; }
    bool		needsRenewal() const	{ return needsrenewal_; }

protected:

    bool		acceptOK(CallBacker*) override;

    ObjectSet<uiSettingsGroup>	grps_;
    Settings&		setts_;
    bool		changed_;
    bool		needsrestart_;
    bool		needsrenewal_;
};


mExpClass(uiTools) uiGeneralSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiGeneralSettingsGroup);
public:
			mDefaultFactoryInstantiation2Param(
				uiSettingsGroup,
				uiGeneralSettingsGroup,
				uiParent*,Settings&,
				"General",
				toUiString(sFactoryKeyword()))

			~uiGeneralSettingsGroup();

    bool		acceptOK() override;

protected:
			uiGeneralSettingsGroup(uiParent*,Settings&);

    uiGenInput*		iconszfld_;
    uiComboBox*		stylefld_;
    uiGenInput*		showinlprogressfld_;
    uiGenInput*		showcrlprogressfld_;
    uiGenInput*		showzprogressfld_;
    uiGenInput*		showrdlprogressfld_;
    uiGenInput*		virtualkeyboardfld_;
    uiGenInput*		nrprocfld_;

    int			iconsz_;
    BufferString	prevstyle_;
    bool		showinlprogress_	= true;
    bool		showcrlprogress_	= true;
    bool		showzprogress_		= true;
    bool		showrdlprogress_	= true;
    bool		enabvirtualkeyboard_	= false;
};


mExpClass(uiTools) uiVisSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiVisSettingsGroup);
public:
			mDefaultFactoryInstantiation2Param(
				uiSettingsGroup,
				uiVisSettingsGroup,
				uiParent*,Settings&,
				"Visualization",
				toUiString(sFactoryKeyword()))

			uiVisSettingsGroup(uiParent*,Settings&);
			~uiVisSettingsGroup();

    bool		acceptOK() override;

protected:

    void		mipmappingToggled(CallBacker*);

    uiComboBox*		textureresfactorfld_;
    uiGenInput*		usesurfshadersfld_;
    uiGenInput*		usevolshadersfld_;
    uiGenInput*		enablemipmappingfld_;
    uiLabeledComboBox*	anisotropicpowerfld_;
    uiMaterialGroup*	matgrp_;

			//0=standard, 1=higher, 2=highest, 3=system default
    int			textureresindex_;
    bool		usesurfshaders_;
    bool		usevolshaders_;
    bool		enablemipmapping_;
    int			anisotropicpower_;
};
