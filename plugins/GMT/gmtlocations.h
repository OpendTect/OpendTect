#ifndef gmtloc_h
#define gmtloc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: gmtlocations.h,v 1.1 2008-08-01 08:28:27 cvsraman Exp $
________________________________________________________________________

-*/

#include "gmtpar.h"


class GMTLocations : public GMTPar
{
public:

    static void		initClass();

    			GMTLocations(const char* nm)
			    : GMTPar(nm)	{}
			GMTLocations(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char*	progName() const		{ return "psxy"; }
    virtual const char* userRef() const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};


class GMTPolyline : public GMTPar
{
public:

    static void		initClass();

    			GMTPolyline(const char* nm)
			    : GMTPar(nm)	{}
			GMTPolyline(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char*	progName() const		{ return "psxy"; }
    virtual const char* userRef() const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};

#endif
