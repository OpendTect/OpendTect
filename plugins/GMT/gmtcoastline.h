#ifndef gmtcoastline_h
#define gmtcoastline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "gmtpar.h"


mClass(GMT) GMTCoastline : public GMTPar
{ mODTextTranslationClass(GMTCoastline);
public:

    static void		initClass();

			GMTCoastline( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir) {}

    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    virtual bool	doExecute(od_ostream&,const char*);

    static GMTPar*	createInstance(const IOPar&,const char*);
    static int		factoryid_;

    bool		makeLLRangeFile(const char*,od_ostream&);
};

#endif
