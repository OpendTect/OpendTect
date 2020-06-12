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

			GMTLocations(const char* nm)
			    : GMTPar(nm)	{}
			GMTLocations(const IOPar& par)
			    : GMTPar(par) {}

    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};


mClass(GMT) GMTPolyline : public GMTPar
{ mODTextTranslationClass(GMTPolyline);
public:

    static void		initClass();

			GMTPolyline(const char* nm)
			    : GMTPar(nm)	{}
			GMTPolyline(const IOPar& par)
			    : GMTPar(par) {}

    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};


mClass(GMT) GMTWells : public GMTPar
{ mODTextTranslationClass(GMTWells);
public:

    static void		initClass();

			GMTWells(const char* nm)
			    : GMTPar(nm)	{}
			GMTWells(const IOPar& par)
			    : GMTPar(par) {}

    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};
