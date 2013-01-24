#ifndef gmt2dlines_h
#define gmt2dlines_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "gmtpar.h"


mClass(GMT) GMT2DLines : public GMTPar
{
public:

    static void		initClass();

    			GMT2DLines(const char* nm)
			    : GMTPar(nm)	{}
			GMT2DLines(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};


mClass(GMT) GMTRandLines : public GMTPar
{
public:

    static void		initClass();

    			GMTRandLines(const char* nm)
			    : GMTPar(nm)	{}
			GMTRandLines(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};


#endif
