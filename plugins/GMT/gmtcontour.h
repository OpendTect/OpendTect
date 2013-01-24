#ifndef gmtcontour_h
#define gmtcontour_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "gmtpar.h"


mClass(GMT) GMTContour : public GMTPar
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

    bool		makeCPT(const char*) const;
};

#endif
