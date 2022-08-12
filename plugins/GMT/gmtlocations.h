#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "gmtpar.h"


mClass(GMT) GMTLocations : public GMTPar
{ mODTextTranslationClass(GMTLocations);
public:

    static void		initClass();

			GMTLocations( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir) {}

    const char*		userRef() const override;
    bool		fillLegendPar(IOPar&) const override;

protected:

    bool		doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&,const char*);
    static int		factoryid_;
};


mClass(GMT) GMTPolyline : public GMTPar
{ mODTextTranslationClass(GMTPolyline);
public:

    static void		initClass();

			GMTPolyline( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir) {}

    const char*		userRef() const override;
    bool		fillLegendPar(IOPar&) const override;

protected:

    bool		doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&,const char*);
    static int		factoryid_;
};


mClass(GMT) GMTWells : public GMTPar
{ mODTextTranslationClass(GMTWells);
public:

    static void		initClass();

			GMTWells( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir) {}

    const char*		userRef() const override;
    bool		fillLegendPar(IOPar&) const override;

protected:

    bool		doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&,const char*);
    static int		factoryid_;
};

