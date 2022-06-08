#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Jun 2003
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "trckeyzsampling.h"
#include "bufstringset.h"
#include "typeset.h"
#include "unitofmeasure.h"


namespace EM
{

class Surface;

/*!
\brief Data interesting for Surface I/O.
*/

mExpClass(EarthModel) SurfaceIOData
{
public:
			 SurfaceIOData() : nrfltsticks_(0){}
    			~SurfaceIOData()	{ clear(); }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    void		clear();
    void		use(const Surface&);

    BufferString		dbinfo;
    TrcKeySampling		rg;			// 3D only
    Interval<float>		zrg;
    BufferStringSet		valnames;
    TypeSet<float>		valshifts_;
    BufferStringSet		sections;
    const UnitOfMeasure*	zunit = UnitOfMeasure::surveyDefZStorageUnit();

    /*! For 2D Only: */
    BufferStringSet		linenames;
    BufferStringSet		linesets;
    TypeSet<Pos::GeomID>	geomids;
    TypeSet<StepInterval<int> > trcranges;
    int				nrfltsticks_;
};


/*!
\brief Surface I/O data selection
*/

mExpClass(EarthModel) SurfaceIODataSelection
{
public:

    				SurfaceIODataSelection( const SurfaceIOData& s )
				    : sd(s)		{}

    const SurfaceIOData&	sd;

    TrcKeySampling		rg;
    TypeSet<int>		selvalues; // Indexes in sd.valnames
    TypeSet<int>		selsections; // Indexes in sd.sections

    BufferStringSet		sellinenames;
    TypeSet<Pos::GeomID>	selgeomids;
    TypeSet<StepInterval<int> >	seltrcranges;

    void			setDefault(); // selects all
};

} // namespace EM

