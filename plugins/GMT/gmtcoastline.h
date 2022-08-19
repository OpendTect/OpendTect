#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
