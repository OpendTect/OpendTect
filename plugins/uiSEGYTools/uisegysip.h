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

    const char*		usrText() const override { return "Scan SEG-Y file(s)";}
    uiDialog*		dialog(uiParent*) override;
    bool		getInfo(uiDialog*,TrcKeyZSampling&,
				Coord crd[3]) override;
    bool		xyInFeet() const override	{ return xyinft_; }
    const char*		iconName() const override	{ return "segy"; }

    void		fillLogPars(IOPar&) const override;
    IOPar*		getCoordSystemPars() const override;
    IOPar*		getImportPars() const override;
    void		startImport(uiParent*,const IOPar&) override;
    const char*		importAskQuestion() const override;
    const uiString	importAskUiQuestion() const;

private:

    IOPar		imppars_;
    BufferString	userfilename_;
    bool		xyinft_			= false;
    RefMan<Coords::CoordSystem> coordsystem_;

};
