#ifndef emregion_h
#define emregion_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2014
 RCS:		$Id$
________________________________________________________________________

-*/



#include "earthmodelmod.h"
#include "trckeyzsampling.h"

namespace EM
{

mExpClass(EarthModel) Region
{
public:
				~Region();

    int				id() const;
    const TrcKeyZSampling&	getBoundingBox() const;
    virtual bool		isInside(const TrcKey&,float z,
					 bool includeborder=true) const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
				Region(Pos::GeomID =-1);

    TrcKeyZSampling		tkzs_;
    int				id_;
    Pos::GeomID			geomid_;

};

} // namespace EM

#endif
