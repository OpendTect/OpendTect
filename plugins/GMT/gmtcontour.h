#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
________________________________________________________________________

-*/

#include "gmtpar.h"


mClass(GMT) GMTContour : public GMTPar
{ mODTextTranslationClass(GMTContour);
public:

    static void		initClass();

			GMTContour(const char* nm)
			    : GMTPar(nm)	{}
			GMTContour(const IOPar& par)
			    : GMTPar(par) {}

    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;

    bool		makeCPT(const char*) const;
};
