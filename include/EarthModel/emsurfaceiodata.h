#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "bufstringset.h"
#include "trckeysampling.h"
#include "typeset.h"


namespace ZDomain { class Info; }
class UnitOfMeasure;

namespace EM
{

class Surface;

/*!
\brief Data interesting for Surface I/O.
*/

mExpClass(EarthModel) SurfaceIOData
{
public:
			    SurfaceIOData();
			    ~SurfaceIOData();

    void		    fillPar(IOPar&) const;
    void		    usePar(const IOPar&);

    void		    clear();
    void		    use(const Surface&);

    void		    setZDomain(const ZDomain::Info&);
    const ZDomain::Info&    zDomain() const;

    BufferString		dbinfo;
    TrcKeySampling		rg;			// 3D only
    Interval<float>		zrg;
    BufferStringSet		valnames;
    TypeSet<float>		valshifts_;
    BufferStringSet		sections;

    /*! For 2D Only: */
    BufferStringSet		linenames;
    BufferStringSet		linesets;
    TypeSet<Pos::GeomID>	geomids;
    TypeSet<StepInterval<int>>	trcranges;
    int				nrfltsticks_		= 0;

protected:
    const ZDomain::Info*	zinfo_;

};


/*!
\brief Surface I/O data selection
*/

mExpClass(EarthModel) SurfaceIODataSelection
{
public:
				SurfaceIODataSelection(const SurfaceIOData&);
				~SurfaceIODataSelection();

    const SurfaceIOData&	sd;

    TrcKeySampling		rg;
    TypeSet<int>		selvalues; // Indexes in sd.valnames
    TypeSet<int>		selsections; // Indexes in sd.sections

    BufferStringSet		sellinenames;
    TypeSet<Pos::GeomID>	selgeomids;
    TypeSet<StepInterval<int>>	seltrcranges;

    void			setDefault(); // selects all
};

} // namespace EM
