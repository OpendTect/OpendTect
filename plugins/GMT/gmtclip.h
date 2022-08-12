#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
________________________________________________________________________

-*/


#include "gmtmod.h"
#include "gmtpar.h"


mExpClass(GMT) GMTClip : public GMTPar
{
public:

    static void		initClass();

			GMTClip( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir) {}

    const char*		userRef() const override;
    bool		isStart() const;
    bool		fillLegendPar(IOPar&) const override;

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&,const char*);
    static int		factoryid_;
};

