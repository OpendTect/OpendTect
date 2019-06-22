#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jun 2003
________________________________________________________________________

-*/

#include "emcommon.h"
#include "trckeysampling.h"
#include "bufstringset.h"
#include "typeset.h"


namespace EM
{

class Surface;

/*!\brief Data interesting for Surface I/O. */

mExpClass(EarthModel) SurfaceIOData
{
public:
			 SurfaceIOData() : nrfltsticks_(0){}
			~SurfaceIOData()	{ clear(); }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    void		clear();
    void		use(const Surface&);

    BufferString	dbinfo;
    TrcKeySampling	rg;			// 3D only
    Interval<float>	zrg;
    BufferStringSet	valnames;
    TypeSet<float>	valshifts_;
    BufferStringSet	sections;

    /*! For 2D Only: */
    BufferStringSet	linenames;
    BufferStringSet	linesets;
    GeomIDSet		geomids;
    TypeSet<StepInterval<int> > trcranges;
    int			nrfltsticks_;
};


/*!
\brief Surface I/O data selection
*/

mExpClass(EarthModel) SurfaceIODataSelection
{
public:

				SurfaceIODataSelection()
				    : sd(*new SurfaceIOData)		{}
				SurfaceIODataSelection( const SurfaceIOData& s )
				    : sd(s), sdmine_(false)		{}
				~SurfaceIODataSelection()
				{ if ( sdmine_ ) delete &sd; }

    const SurfaceIOData&	sd;

    TrcKeySampling		rg;
    TypeSet<int>		selvalues; // Indexes in sd.valnames

    BufferStringSet		sellinenames;
    GeomIDSet			selgeomids;
    TypeSet<StepInterval<int> >	seltrcranges;

    void			setDefault(); // selects all

protected:

    bool			sdmine_		= true;

};

} // namespace EM
