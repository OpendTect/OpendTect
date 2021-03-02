#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Feb 2021
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
