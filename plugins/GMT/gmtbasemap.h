#ifndef gmtbasemap_h
#define gmtbasemap_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: gmtbasemap.h,v 1.2 2008-08-06 09:58:20 cvsraman Exp $
________________________________________________________________________

-*/

#include "gmtpar.h"


class GMTBaseMap : public GMTPar
{
public:

    static void		initClass();

    			GMTBaseMap(const char* nm)
			    : GMTPar(nm)	{}
			GMTBaseMap(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const			{ return 0; }

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};


class GMTLegend : public GMTPar
{
public:

    static void		initClass();

			GMTLegend(const char* nm)
			    : GMTPar(nm)	{}
			GMTLegend(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const			{ return 0; }

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};

#endif
