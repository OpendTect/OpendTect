#ifndef emsurfaceiodata_h
#define emsurfaceiodata_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert Bril
 Date:		Jun 2003
 RCS:		$Id: emsurfaceiodata.h,v 1.2 2003-10-17 14:19:00 bert Exp $
________________________________________________________________________

-*/

#include "binidselimpl.h"
#include "bufstringset.h"


namespace EM
{

class Surface;

/*!\brief Data interesting for Surface I/O */

class SurfaceIOData
{
public:

    			~SurfaceIOData()	{ clear(); }

    void		clear();
    void		use(const Surface&);

    BufferString	dbinfo;
    BinIDSampler	rg;
    BufferStringSet	valnames;
    BufferStringSet	patches;

};


class SurfaceIODataSelection
{
public:

    				SurfaceIODataSelection( const SurfaceIOData& s )
				    : sd(s)		{}

    const SurfaceIOData&	sd;

    BinIDSampler		rg;
    TypeSet<int>		selvalues; // Indexes in sd.valnames
    TypeSet<int>		selpatches; // Indexes in sd.patches

    void			setDefault(); // selects all
};



}; // Namespace

#endif
