#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "coordsystem.h"
#include "factory.h"
#include "uicompoundparsel.h"
#include "uidlggroup.h"

class SurveyInfo;
class uiGenInput;
class uiLabel;
class uiCheckBox;

namespace Coords
{

mExpClass(uiTools) uiPositionSystem : public uiDlgGroup
{
public:
    mDefineFactory1ParamInClass(uiPositionSystem,uiParent*,factory);

    virtual bool		initFields(const PositionSystem*)= 0;

    RefMan<PositionSystem>	outputSystem() { return outputsystem_; }
				//!<After AcceptOK();

    virtual HelpKey		helpKey() const { return helpkey_; }

    void			setSurveyInfo( const SurveyInfo* si )
				{ si_ = si; }

protected:
				uiPositionSystem(uiParent*,const uiString&);
    RefMan<PositionSystem>	outputsystem_;
    HelpKey			helpkey_;
    const SurveyInfo*		si_;
};


mExpClass(uiTools) uiPositionSystemSelGrp : public uiDlgGroup
{ mODTextTranslationClass(uiPositionSystemSel);
public:
				uiPositionSystemSelGrp(uiParent*,
						bool onlyorthogonal,
						bool onlyprojection,
						const SurveyInfo*,
						const Coords::PositionSystem*);
				~uiPositionSystemSelGrp();
    RefMan<PositionSystem>	outputSystem() { return outputsystem_; }
				//!<After AcceptOK();
    bool			acceptOK();

private:

    void			systemChangedCB(CallBacker*);

    uiGenInput*			coordsystemsel_;
    uiLabel*			coordsystemdesc_;
    ObjectSet<uiPositionSystem> coordsystemsuis_;
    ManagedObjectSet<IOPar>	coordsystempars_;
    const SurveyInfo*		si_;

    RefMan<PositionSystem>	outputsystem_;
};


mExpClass(uiTools) uiPositionSystemDlg : public uiDialog
{ mODTextTranslationClass(uiPositionSystemDlg);
public:
			uiPositionSystemDlg(uiParent*,bool orthogonalonly,
				bool projectiononly,const PositionSystem*);
			~uiPositionSystemDlg();

    RefMan<PositionSystem> getCoordSystem();

    static bool		ensureGeographicTransformOK(uiParent*,SurveyInfo* si=0);

protected:

    uiPositionSystemSelGrp* coordsysselfld_;

    bool		acceptOK(CallBacker*);

};


mExpClass(uiTools) uiPositionSystemSel : public uiCompoundParSel
{ mODTextTranslationClass(uiPositionSystemSel);
public:
			uiPositionSystemSel(uiParent*,const uiString& seltxt,
					bool orthogonalonly,bool projectiononly,
					const PositionSystem*);

    RefMan<PositionSystem> getCoordSystem()	{ return coordsystem_; }

protected:

    uiPositionSystemDlg* dlg_;

    RefMan<PositionSystem> coordsystem_;
    bool		orthogonalonly_;
    bool		projectiononly_;

    BufferString	getSummary() const;
    void		selCB(CallBacker*);

};

} //Namespace
