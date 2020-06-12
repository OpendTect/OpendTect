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

			GMTClip(const char* nm)
			    : GMTPar(nm)	{}
			GMTClip(const IOPar& par)
			    : GMTPar(par) {}

    virtual const char* userRef() const;
    bool		isStart() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};
