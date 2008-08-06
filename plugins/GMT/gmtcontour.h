#ifndef gmtcontour_h
#define gmtcontour_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: gmtcontour.h,v 1.1 2008-08-06 09:58:20 cvsraman Exp $
________________________________________________________________________

-*/

#include "gmtpar.h"


class GMTContour : public GMTPar
{
public:

    static void		initClass();

    			GMTContour(const char* nm)
			    : GMTPar(nm)	{}
			GMTContour(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};

#endif
