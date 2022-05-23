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
#include "survinfo.h"

class SurveyInfo;
class uiGenInput;
class uiLabel;
class uiCheckBox;

namespace Coords
{

mExpClass(uiTools) uiCoordSystem : public uiDlgGroup
{
public:
    mDefineFactory1ParamInClass(uiCoordSystem,uiParent*,factory);

    virtual bool		initFields(const CoordSystem*)= 0;

    RefMan<CoordSystem>	outputSystem() { return outputsystem_; }
				//!<After AcceptOK();

    virtual HelpKey		helpKey() const { return helpkey_; }

    void			setSurveyInfo( const SurveyInfo* si )
				{ si_ = si; }

protected:
				uiCoordSystem(uiParent*,const uiString&);
    RefMan<CoordSystem>	outputsystem_;
    HelpKey			helpkey_;
    const SurveyInfo*		si_;
};


mExpClass(uiTools) uiCoordSystemSelGrp : public uiDlgGroup
{ mODTextTranslationClass(uiCoordSystemSel);
public:
				uiCoordSystemSelGrp(uiParent*,
						bool orthogonal,
						bool onlyprojection,
						const SurveyInfo*,
						const Coords::CoordSystem*);
				~uiCoordSystemSelGrp();

    RefMan<CoordSystem>		outputSystem() { return outputsystem_; }
				//!<After AcceptOK();
    bool			acceptOK();
    void			fillFromSI();
    void			fillFrom(const Coords::CoordSystem&);

private:

    void			createGeodeticUI();
    void			wgs84Sel(CallBacker*);
    void			systemChangedCB(CallBacker*);

    uiGenInput*			coordsystemsel_		= nullptr;
    uiGenInput*			wgs84selfld_		= nullptr;
    ObjectSet<uiCoordSystem>	coordsystemsuis_;

    ManagedObjectSet<IOPar>	coordsystempars_;
    const SurveyInfo*		si_;

    RefMan<CoordSystem>		outputsystem_;
};


mExpClass(uiTools) uiCoordSystemDlg : public uiDialog
{ mODTextTranslationClass(uiCoordSystemDlg);
public:
			uiCoordSystemDlg(uiParent*,bool orthogonal,
					bool projectiononly,const SurveyInfo*,
					const CoordSystem*);
			~uiCoordSystemDlg();

    RefMan<CoordSystem> getCoordSystem();

    static bool		ensureGeographicTransformOK(uiParent*,SurveyInfo* si=0);

protected:

    uiCoordSystemSelGrp* coordsysselfld_;

    bool		acceptOK(CallBacker*);

};


mExpClass(uiTools) uiCoordSystemSel : public uiCompoundParSel
{ mODTextTranslationClass(uiCoordSystemSel);
public:
			uiCoordSystemSel(uiParent*,
				bool orthogonal=true,
				bool projectiononly=true,
				const CoordSystem* crs=SI().getCoordSystem(),
				const uiString& seltxt=uiStrings::sCoordSys());
			~uiCoordSystemSel();

    RefMan<CoordSystem>		getCoordSystem()	{ return coordsystem_; }

    Notifier<uiCoordSystemSel>	changed;

protected:

    uiCoordSystemDlg*		dlg_ = nullptr;

    RefMan<CoordSystem>		coordsystem_;
    bool			orthogonal_;
    bool			projectiononly_;

    BufferString		getSummary() const;
    void			selCB(CallBacker*);

};


mExpClass(uiTools) uiLatLongSystemSel : public uiCoordSystemSel
{
public:
			uiLatLongSystemSel(uiParent*,const uiString& seltxt,
				ConstRefMan<Coords::CoordSystem> cs=nullptr);

    bool		isWGS84() const;

};

} // namespace Coords
