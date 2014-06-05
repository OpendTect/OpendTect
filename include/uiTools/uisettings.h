#ifndef uisettings_h
#define uisettings_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2004
 RCS:		$Id$
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "uidialog.h"
#include "uidlggroup.h"
#include "factory.h"

class Settings;
class uiComboBox;
class uiGenInput;
class uiTable;


mExpClass(uiTools) uiSettings : public uiDialog
{ mODTextTranslationClass(uiSettings);
public:
			uiSettings(uiParent*,const char* titl,
				   const char* settskey=0);
    virtual		~uiSettings();

			// Specify this to edit the survey defaults
    static const char*	sKeySurveyDefs()	{ return "SurvDefs"; }

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
    bool		acceptOK(CallBacker*);

};


mExpClass(uiTools) uiSettingsGroup : public uiDlgGroup
{
public:
			mDefineFactory2ParamInClass(uiSettingsGroup,
						    uiParent*,Settings&,
						    factory)
    virtual		~uiSettingsGroup();

    bool		isChanged() const	{ return changed_; }
    const char*		errMsg() const;

protected:
			uiSettingsGroup(uiParent*,const uiString& caption,
					Settings&);

    void		updateSettings(bool oldval,bool newval,const char* key);

    BufferString	errmsg_;
    Settings&		setts_;
    bool		changed_;
};


mExpClass(uiTools) uiSettingsDlg : public uiTabStackDlg
{
public:
			uiSettingsDlg(uiParent*);
			~uiSettingsDlg();

    bool		isChanged() const	{ return changed_; }

protected:

    bool		acceptOK(CallBacker*);

    ObjectSet<uiSettingsGroup>	grps_;
    Settings&		setts_;
    bool		changed_;
};


mExpClass(uiTools) uiGeneralSettingsGroup : public uiSettingsGroup
{
public:
			mDefaultFactoryInstantiation2Param(
				uiSettingsGroup,
				uiGeneralSettingsGroup,
				uiParent*,Settings&,
				"General",
				sFactoryKeyword())

			uiGeneralSettingsGroup(uiParent*,Settings&);
    bool		acceptOK();

protected:

    uiGenInput*		iconszfld_;
    uiGenInput*		colbarhvfld_;
    uiGenInput*		showinlprogressfld_;
    uiGenInput*		showcrlprogressfld_;

    int			iconsz_;
    bool		vertcoltab_;
    bool		showinlprogress_;
    bool		showcrlprogress_;
};


mExpClass(uiTools) uiVisSettingsGroup : public uiSettingsGroup
{
public:
			mDefaultFactoryInstantiation2Param(
				uiSettingsGroup,
				uiVisSettingsGroup,
				uiParent*,Settings&,
				"Visualisation",
				sFactoryKeyword())

			uiVisSettingsGroup(uiParent*,Settings&);
    bool		acceptOK();

protected:

    void		shadersChange(CallBacker*);

    uiComboBox*		textureresfactorfld_;
    uiGenInput*		usesurfshadersfld_;
    uiGenInput*		usevolshadersfld_;

		  // -1: system default, 0 - standard, 1 - higher, 2 - highest
    int			textureresfactor_;
    bool		usesurfshaders_;
    bool		usevolshaders_;
};

#endif
