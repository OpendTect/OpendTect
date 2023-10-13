#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uisip.h"

namespace Coords { class CoordSystem; }
namespace Survey { class Geometry2D; }

mExpClass(uiIo) ui2DSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(ui2DSurvInfoProvider);
public:
			ui2DSurvInfoProvider();
			~ui2DSurvInfoProvider();

    const char*		usrText() const override
			{ return "Enter X/Y ranges"; }
    uiDialog*		dialog(uiParent*) override;
    bool		getInfo(uiDialog*,TrcKeyZSampling&,
				Coord crd[3]) override;

    void		fillLogPars(IOPar&) const override;
    IOPar*		getCoordSystemPars() const override;

    bool		xyInFeet() const override	{ return xyft_; }
    const char*		iconName() const override
			{ return "seismicline2dcollection"; }

protected:

    bool		xyft_		= false;
};



mExpClass(uiIo) uiNavSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(uiNavSurvInfoProvider);
public:
			uiNavSurvInfoProvider();
    virtual		~uiNavSurvInfoProvider();

    const char*		usrText() const override;
    uiDialog*		dialog(uiParent*) override;
    bool		getInfo(uiDialog*,TrcKeyZSampling&,
				Coord crd[3]) override;
    const char*		iconName() const override;

    void		fillLogPars(IOPar&) const override;
    IOPar*		getImportPars() const override;
    void		startImport(uiParent*,const IOPar&) override;
    const char*		importAskQuestion() const override;

    IOPar*		getCoordSystemPars() const override;

protected:
    RefMan<Coords::CoordSystem>		coordsystem_;
    ObjectSet<Survey::Geometry2D>	geoms_;
    BufferString			filename_;
};
