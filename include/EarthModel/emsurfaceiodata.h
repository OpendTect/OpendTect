#ifndef emsurfaceiodata_h
#define emsurfaceiodata_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert Bril
 Date:		Jun 2003
 RCS:		$Id: emsurfaceiodata.h,v 1.1 2003-06-19 13:38:32 bert Exp $
________________________________________________________________________


-*/

#include "binidselimpl.h"
#include "bufstring.h"
#include "sets.h"

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

    BufferString		dbinfo;
    BinIDSampler		rg;
    ObjectSet<BufferString>	valnames;
    ObjectSet<BufferString>	patches;

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
