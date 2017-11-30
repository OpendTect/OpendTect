/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"
class uiGenInput;
class uiCheckList;
class uiListBox;
class SurveyInfo;
class uiSurvInfoProvider;


mExpClass(uiIo) uiUserCreateSurvey : public uiDialog
{ mODTextTranslationClass(uiUserCreateSurvey);

public:

			uiUserCreateSurvey(uiParent*,const char* dataroot=0);
			~uiUserCreateSurvey();

    uiString		sipName() const;
    BufferString	dirName() const;
    BufferString	survName() const;
    BufferString	survDirName() const;
    OD::Pol2D3D		pol2D3D() const;
    const uiSurvInfoProvider* getSIP() const;

protected:

    ObjectSet<uiSurvInfoProvider> sips_;
    const BufferString	dataroot_;
    SurveyInfo*		survinfo_;

    uiGenInput*		survnmfld_;
    uiGenInput*		zistimefld_;
    uiGenInput*		zinfeetfld_;
    uiCheckList*	pol2d3dfld_;
    uiListBox*		sipfld_;

    bool		acceptOK();

    uiRetVal		getDefSurvInfo();
    bool		has3D() const;
    bool		has2D() const;
    bool		isTime() const;
    bool		isInFeet() const;

    bool		usrInputOK();
    void		fillSipsFld(bool have2d,bool have3d);
    void		pol2d3dChg(CallBacker*);
    void		zdomainChg(CallBacker*);
    bool		doUsrDef();

};
