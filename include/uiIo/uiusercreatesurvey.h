/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"
#include "survinfo.h"
class uiGenInput;
class uiCheckList;
class uiListBox;
class uiSurvInfoProvider;


mExpClass(uiIo) uiUserCreateSurvey : public uiDialog
{ mODTextTranslationClass(uiUserCreateSurvey);

public:

			uiUserCreateSurvey(uiParent*,const char* dataroot=0);
			~uiUserCreateSurvey();

    SurveyInfo*		getSurvInfo()
			{ SurveyInfo* si = survinfo_; survinfo_ = 0; return si;}
    int			sipIdx() const	    { return sipidx_; }

protected:

    ObjectSet<uiSurvInfoProvider> sips_;
    const BufferString	dataroot_;
    SurveyInfo*		survinfo_;
    int			sipidx_;

    uiGenInput*		survnmfld_;
    uiGenInput*		zistimefld_;
    uiGenInput*		zinfeetfld_;
    uiCheckList*	pol2dfld_;
    uiListBox*		sipfld_;

    bool		acceptOK();

    uiRetVal		getDefSurvInfo();
    BufferString	sipName() const;
    BufferString	survName() const;
    bool		has3D() const;
    bool		has2D() const;
    bool		isTime() const;
    bool		isInFeet() const;

    bool		usrInputOK();
    void		fillSipsFld(bool have2d,bool have3d);
    SurveyInfo::Pol2D	pol2D() const;
    void		pol2dChg(CallBacker*);
    void		zdomainChg(CallBacker*);
    bool		doUsrDef();

};
