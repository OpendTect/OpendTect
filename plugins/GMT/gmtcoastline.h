#ifndef gmtcoastline_h
#define gmtcoastline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: gmtcoastline.h,v 1.1 2008-08-18 11:22:43 cvsraman Exp $
________________________________________________________________________

-*/

#include "gmtpar.h"


class GMTCoastline : public GMTPar
{
public:

    static void		initClass();

    			GMTCoastline(const char* nm)
			    : GMTPar(nm)	{}
			GMTCoastline(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;

    bool		makeLLRangeFile(const char*) const;
};

#endif
