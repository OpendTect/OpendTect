#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2010
________________________________________________________________________

-*/

#include "gmtmod.h"
#include "gmtpar.h"

class BufferStringSet;
class Coord3ListImpl;
namespace Geometry { class ExplFaultStickSurface; }
namespace EM { class Fault3D; }

mExpClass(GMT) GMTFault : public GMTPar
{ mODTextTranslationClass(GMTFault);
public:
    static void		initClass();

			GMTFault( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir)	{}

    const char*		userRef() const override;
    bool		fillLegendPar(IOPar&) const override;

protected:

    bool		doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&,const char*);
    TypeSet<Coord3>	getCornersOfZSlice(float) const;
    bool		calcOnHorizon(const Geometry::ExplFaultStickSurface&,
				      Coord3ListImpl&) const;

    static int		factoryid_;
    void		getLineStyles(BufferStringSet&);
    bool		loadFaults(uiString&);
    ObjectSet<EM::Fault3D>	flts_;
};

