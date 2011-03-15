#ifndef gmtfault_h
#define gmtfault_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2010
 RCS:           $Id: gmtfault.h,v 1.4 2011-03-15 17:15:12 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "gmtpar.h"

class Coord3;
class Coord3ListImpl;
namespace Geometry { class ExplFaultStickSurface; }

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
    TypeSet<Coord3>	getCornersOfZSlice(float) const;
    bool		calcOnHorizon(const Geometry::ExplFaultStickSurface&,
	    			      Coord3ListImpl&) const;

    static int		factoryid_;
};

#endif
