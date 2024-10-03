#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uisip.h"

#include "coordsystem.h"

/* uiSurvInfoProvider taking its source in (a) SEG-Y file(s) */

mExpClass(uiSEGYTools) uiSEGYSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(uiSEGYSurvInfoProvider)
public:

			uiSEGYSurvInfoProvider();
			~uiSEGYSurvInfoProvider();
			mOD_DisableCopy(uiSEGYSurvInfoProvider);

    uiString		usrText() const override
			{ return tr("Scan SEG-Y file(s)");}
    uiDialog*		dialog(uiParent*) override;
    bool		getInfo(uiDialog*,TrcKeyZSampling&,
				Coord crd[3]) override;
    bool		xyInFeet() const override	{ return xyinft_; }
    const char*		iconName() const override	{ return "segy"; }
    TDInfo		tdInfo() const override		{ return tdinfo_; }

    void		fillLogPars(IOPar&) const override;
    IOPar*		getCoordSystemPars() const override;
    IOPar*		getImportPars() const override;
    void		startImport(uiParent*,const IOPar&) override;
    uiString		importAskQuestion() const override;
    const uiString	importAskUiQuestion() const;

private:

    TDInfo		tdinfo_				= Unknown;
    IOPar		imppars_;
    BufferString	userfilename_;
    bool		xyinft_				= false;
    ConstRefMan<Coords::CoordSystem> coordsystem_;

};

mExternC(uiSEGYTools) void uiSEGYToolsInitSIP();
