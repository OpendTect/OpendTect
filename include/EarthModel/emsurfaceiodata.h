#ifndef emsurfaceiodata_h
#define emsurfaceiodata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Jun 2003
 RCS:		$Id$
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "cubesampling.h"
#include "bufstringset.h"
#include "typeset.h"


namespace EM
{

class Surface;

/*!
\brief Data interesting for Surface I/O.
*/

mExpClass(EarthModel) SurfaceIOData
{
public:
    			~SurfaceIOData()	{ clear(); }

    void                fillPar(IOPar&) const;
    void                usePar(const IOPar&);

    void		clear();
    void		use(const Surface&);

    BufferString	dbinfo;
    HorSampling		rg;			// 3D only
    Interval<float>	zrg;
    BufferStringSet	valnames;
    TypeSet<float>	valshifts_;
    BufferStringSet	sections;

    /*! For 2D Only: */
    BufferStringSet	linenames;
    BufferStringSet	linesets;
    TypeSet<Pos::GeomID>	geomids;
    TypeSet<StepInterval<int> > trcranges;
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

    HorSampling			rg;
    TypeSet<int>		selvalues; // Indexes in sd.valnames
    TypeSet<int>		selsections; // Indexes in sd.sections

    BufferStringSet		sellinenames;
    TypeSet<Pos::GeomID>	selgeomids;
    TypeSet<StepInterval<int> >	seltrcranges;

    void			setDefault(); // selects all
};



}; // namespace EM

#endif

