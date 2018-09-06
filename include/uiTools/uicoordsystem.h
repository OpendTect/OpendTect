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
class uiLatLongInp;

namespace Coords
{

mExpClass(uiTools) uiCoordSystem : public uiDlgGroup
{
public:

    mDefineFactory1ParamInClass(uiCoordSystem,uiParent*,factory)

    virtual bool		initFields(const CoordSystem*)= 0;

    RefMan<CoordSystem>	outputSystem() { return outputsystem_; }
				//!<After AcceptOK();

    virtual HelpKey		helpKey() const { return helpkey_; }

    void			setSurveyInfo( const SurveyInfo* si )
				{ si_ = si; }

protected:
				uiCoordSystem(uiParent*,const uiString&);

    RefMan<CoordSystem>		outputsystem_;
    HelpKey			helpkey_;
    const SurveyInfo*		si_;
};


mExpClass(uiTools) uiCoordSystemSelGrp : public uiDlgGroup
{ mODTextTranslationClass(uiCoordSystemSel)
public:
				uiCoordSystemSelGrp(uiParent*,
						bool onlyorthogonal,
						bool onlyprojection,
						const SurveyInfo*,
						const Coords::CoordSystem*);
				~uiCoordSystemSelGrp();
    RefMan<CoordSystem>		outputSystem() { return outputsystem_; }
				//!<After AcceptOK();
    bool			acceptOK();

private:

    void			systemChangedCB(CallBacker*);

    uiGenInput*			coordsystemsel_;
    ObjectSet<uiCoordSystem>	coordsystemsuis_;
    ManagedObjectSet<IOPar>	coordsystempars_;
    const SurveyInfo*		si_;

    RefMan<CoordSystem>		outputsystem_;
};


mExpClass(uiTools) uiCoordSystemDlg : public uiDialog
{ mODTextTranslationClass(uiCoordSystemDlg)
public:
			uiCoordSystemDlg(uiParent*,bool orthogonalonly,
				bool projectiononly,const CoordSystem*);
			~uiCoordSystemDlg();

    RefMan<CoordSystem> getCoordSystem();

    static bool		ensureGeographicTransformOK(uiParent*,SurveyInfo* si=0);

protected:

    uiCoordSystemSelGrp* coordsysselfld_;

    bool		acceptOK();
};


mExpClass(uiTools) uiCoordSystemSel : public uiCompoundParSel
{ mODTextTranslationClass(uiCoordSystemSel)
public:
			uiCoordSystemSel(uiParent*,
				bool orthogonalonly=true,
				bool projectiononly=true,
				const CoordSystem* crs=SI().getCoordSystem(),
				const uiString& seltxt=uiStrings::sCoordSys());

    RefMan<CoordSystem> getCoordSystem()	{ return coordsystem_; }

protected:

    uiCoordSystemDlg*	dlg_;

    RefMan<CoordSystem>	coordsystem_;
    bool		orthogonalonly_;
    bool		projectiononly_;

    uiString		getSummary() const;
    void		selCB(CallBacker*);
};


mExpClass(uiTools) uiUnlocatedXYSystem : public uiCoordSystem
{ mODTextTranslationClass(uiUnlocatedXYSystem);
public:

    mDefaultFactoryInstantiation1Param( uiCoordSystem, uiUnlocatedXYSystem,
			       uiParent*, UnlocatedXY::sFactoryKeyword(),
			       UnlocatedXY::sFactoryDisplayName() )

			uiUnlocatedXYSystem(uiParent*);


    virtual bool	initFields(const CoordSystem*);

protected:

    uiGenInput*		xyunitfld_;

    bool		acceptOK();
};


mExpClass(uiTools) uiAnchorBasedXYSystem : public uiCoordSystem
{ mODTextTranslationClass(uiAnchorBasedXYSystem)
public:
    mDefaultFactoryInstantiation1Param( uiCoordSystem, uiAnchorBasedXYSystem,
			       uiParent*, AnchorBasedXY::sFactoryKeyword(),
			       AnchorBasedXY::sFactoryDisplayName() )

			uiAnchorBasedXYSystem(uiParent*);

    virtual bool	initFields(const CoordSystem*);

protected:

    uiGenInput*		coordfld_;
    uiLatLongInp*	latlngfld_;
    uiGenInput*		xyunitfld_;

    bool		acceptOK();
};

} // namespace Coords
