#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    ConstRefMan<CoordSystem>	outputSystem() const;
				//!<After AcceptOK();

    HelpKey			helpKey() const override { return helpkey_; }

    void			setSurveyInfo( const SurveyInfo* si )
				{ si_ = si; }

protected:
				uiCoordSystem(uiParent*,const uiString&);
				~uiCoordSystem();

    ConstRefMan<CoordSystem>	outputsystem_;
    HelpKey			helpkey_;
    const SurveyInfo*		si_ = nullptr;
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

    bool			hasOutputSystem() const;
    ConstRefMan<CoordSystem>	outputSystem() const;
				//!<After AcceptOK();

    bool			acceptOK() override;
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

    ConstRefMan<CoordSystem>	outputsystem_;
};


mExpClass(uiTools) uiCoordSystemDlg : public uiDialog
{ mODTextTranslationClass(uiCoordSystemDlg);
public:
			uiCoordSystemDlg(uiParent*,bool orthogonal,
					 bool projectiononly,const SurveyInfo*,
					 const CoordSystem*);
			~uiCoordSystemDlg();

    ConstRefMan<CoordSystem> getCoordSystem() const;

    static bool		ensureGeographicTransformOK(uiParent*,
						    SurveyInfo* =nullptr);

protected:

    uiCoordSystemSelGrp* coordsysselfld_;

    bool		acceptOK(CallBacker*) override;

};


mExpClass(uiTools) uiCoordSystemSel : public uiCompoundParSel
{ mODTextTranslationClass(uiCoordSystemSel);
public:
			uiCoordSystemSel(uiParent*,
				bool orthogonal=true,
				bool projectiononly=true,
				const CoordSystem* crs=
					SI().getCoordSystem().ptr(),
				const uiString& seltxt=uiStrings::sCoordSys());
			~uiCoordSystemSel();

    void			setCoordSystem(const CoordSystem*);
    void			doSel();

    ConstRefMan<CoordSystem>	getCoordSystem() const;

    Notifier<uiCoordSystemSel>	changed;

protected:

    uiCoordSystemDlg*		dlg_ = nullptr;

    ConstRefMan<CoordSystem>	coordsystem_;
    bool			orthogonal_;
    bool			projectiononly_;

    BufferString		getSummary() const override;
    void			selCB(CallBacker*);

};


mExpClass(uiTools) uiLatLongSystemSel : public uiCoordSystemSel
{
public:
			uiLatLongSystemSel(uiParent*,const uiString& seltxt,
				ConstRefMan<Coords::CoordSystem> cs=nullptr);
			~uiLatLongSystemSel();

    bool		isWGS84() const;

};

} // namespace Coords
