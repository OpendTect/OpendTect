#ifndef gmtfault_h
#define gmtfault_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2010
 RCS:           $Id: gmtfault.h,v 1.1 2010-04-07 09:26:18 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "gmtpar.h"

mClass GMTFault : public GMTPar
{
public:
    static void		initClass();

    			GMTFault(const char* nm)
			    : GMTPar(nm)	{}
			GMTFault(const IOPar& par)
			    : GMTPar(par)	{}

    bool		execute(std::ostream&,const char*);
    const char*		userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:
    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};

#endif
