#ifndef uisettings_h
#define uisettings_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2004
 RCS:		$Id: uisettings.h,v 1.21 2012-08-03 13:01:15 cvskris Exp $
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "uidialog.h"

class IOPar;
class Settings;
class uiTable;
class uiGenInput;
class uiLabeledComboBox;
struct LooknFeelSettings;


mClass(uiTools) uiSettings : public uiDialog
{
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


mClass(uiTools) uiLooknFeelSettings : public uiDialog
{
public:
			uiLooknFeelSettings(uiParent*,const char* titl);
    virtual		~uiLooknFeelSettings();

    bool		isChanged() const	{ return changed_; }

protected:

    Settings&		setts_;
    LooknFeelSettings&	lfsetts_;
    bool		changed_;

    uiGenInput*		iconszfld_;
    uiGenInput*		colbarhvfld_;
    uiGenInput*		colbarontopfld_;
    uiGenInput*		showwheelsfld_;
    uiGenInput*		showinlprogressfld_;
    uiGenInput*		showcrlprogressfld_;
    uiLabeledComboBox*	textureresfactorfld_;
    uiGenInput*		useshadingfld_;
    uiGenInput*		volrenshadingfld_;

    void		updateSettings(bool oldval,bool newval,const char* key);

    void		shadingChange(CallBacker*);
    bool		acceptOK(CallBacker*);
};

#endif

