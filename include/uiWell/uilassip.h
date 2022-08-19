#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uisip.h"

#include "coordsystem.h"

/*\brief Survey info provider using LAS format file


 */

mExpClass(uiWell) uiLASSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(uiLASSurvInfoProvider);
public:
				uiLASSurvInfoProvider();
				~uiLASSurvInfoProvider();

				uiLASSurvInfoProvider(
					const uiLASSurvInfoProvider&) =delete;
    uiLASSurvInfoProvider&	operator=(const uiLASSurvInfoProvider&) =delete;

    const char*			usrText() const override;
    uiDialog*			dialog(uiParent*) override;
    bool			getInfo(uiDialog*,TrcKeyZSampling&,
							Coord crd[3]) override;
    const char*			iconName() const override;

    void			fillLogPars(IOPar&) const override;
    IOPar*			getImportPars() const override;
    void			startImport(uiParent*,const IOPar&) override;
    const char*			importAskQuestion() const override;

    IOPar*			getCoordSystemPars() const override;

protected:
    RefMan<Coords::CoordSystem>		coordsystem_;
    BufferStringSet			filenms_;
};
