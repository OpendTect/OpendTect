#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Oct 2004
 RCS:           $Id$
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

    virtual const char*	usrText() const		{ return "Set for 2D only"; }
    virtual uiDialog*	dialog(uiParent*);
    virtual bool	getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);

    virtual void	fillLogPars(IOPar&) const;

    virtual bool	xyInFeet() const	{ return xyft_; }
    virtual const char*	iconName() const   { return "seismicline2dcollection"; }

protected:

    bool		xyft_		= false;
};



mExpClass(uiIo) uiNavSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(uiNavSurvInfoProvider);
public:
				uiNavSurvInfoProvider();
    virtual			~uiNavSurvInfoProvider();

    virtual const char*		usrText() const;
    virtual uiDialog*		dialog(uiParent*);
    virtual bool		getInfo(uiDialog*,TrcKeyZSampling&,
					Coord crd[3]);
    virtual const char*		iconName() const;

    virtual void		fillLogPars(IOPar&) const;
    virtual IOPar*		getImportPars() const;
    virtual void		startImport(uiParent*,const IOPar&);
    virtual const char*		importAskQuestion() const;

    virtual IOPar*		getCoordSystemPars() const;

protected:
    RefMan<Coords::CoordSystem>		coordsystem_;
    ObjectSet<Survey::Geometry2D>	geoms_;
};

