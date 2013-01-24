#ifndef gmtloc_h
#define gmtloc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "gmtpar.h"


mClass(GMT) GMTLocations : public GMTPar
{
public:

    static void		initClass();

    			GMTLocations(const char* nm)
			    : GMTPar(nm)	{}
			GMTLocations(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};


mClass(GMT) GMTPolyline : public GMTPar
{
public:

    static void		initClass();

    			GMTPolyline(const char* nm)
			    : GMTPar(nm)	{}
			GMTPolyline(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};


mClass(GMT) GMTWells : public GMTPar
{
public:

    static void		initClass();

    			GMTWells(const char* nm)
			    : GMTPar(nm)	{}
			GMTWells(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};

#endif
