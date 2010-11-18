#ifndef uisettings_h
#define uisettings_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2004
 RCS:		$Id: uisettings.h,v 1.18 2010-11-18 19:16:04 cvskarthika Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

class IOPar;
class Settings;
class uiGenInput;
class uiLabeledComboBox;
struct LooknFeelSettings;


mClass uiSettings : public uiDialog
{
public:
			uiSettings(uiParent*,const char* titl,
				   const char* settskey=0);
    virtual		~uiSettings();

    			// Specify this to edit the survey defaults
    static const char*	sKeySurveyDefs()	{ return "SurvDefs"; }

protected:

    bool		issurvdefs_; // must be before decl of setts_
    IOPar&		setts_;

    uiGenInput*		keyfld_;
    uiGenInput*		valfld_;

    void		selPush(CallBacker*);
    bool		acceptOK(CallBacker*);

};


mClass uiLooknFeelSettings : public uiDialog
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
