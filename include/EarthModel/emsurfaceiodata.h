#ifndef emsurfaceiodata_h
#define emsurfaceiodata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Jun 2003
 RCS:		$Id: emsurfaceiodata.h,v 1.4 2004-03-03 15:33:30 arend Exp $
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

    void                fillPar(IOPar&) const;
    void                usePar(const IOPar&);

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
