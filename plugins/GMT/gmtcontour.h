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
{ mODTextTranslationClass(GMTContour);
public:

    static void		initClass();

			GMTContour( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir) {}

    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&,const char*);
    static int		factoryid_;

    bool		makeCPT(const char*) const;
};

#endif
