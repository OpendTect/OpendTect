#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "coltabmapper.h"
#include "typeset.h"

class TaskRunner;

namespace dGBVis
{

/*!\brief Maps data values to color indices, like the OpenSceneGraph display.

  The mapper holds the range, clipping, symmetry, segments and flip. The
  values become indices from 0 to nrSteps()-1, undefined values get the index
  nrSteps().
*/

mExpClass(vsgBase) ColTabMapping
{
public:
				ColTabMapping();
				~ColTabMapping();

    static int			nrSteps()		{ return 255; }

    ColTab::Mapper&		mapper()		{ return mapper_; }
    const ColTab::Mapper&	mapper() const		{ return mapper_; }

    void			setData(const float*,od_int64 sz,
					TaskRunner* =nullptr);
				//!<The data must stay until update()
    void			update(bool full=true,TaskRunner* =nullptr);
				//!<After a change of the data or the setup.
				//!<If !full, the clipping is not redone.

    static int			nrBands()		{ return 2; }
    od_int64			size() const		{ return datasz_; }
    const unsigned char*	indices() const		{ return indices_.arr(); }
				/*!<Cell by cell: the index, and a band that is
				    255 for a defined and 0 for an undefined
				    value, as in OpenSceneGraph. */
    unsigned char		indexAt(od_int64 pos) const
				{ return indices_[ int(pos*nrBands()) ]; }
    bool			isUndefAt(od_int64 pos) const
				{ return indices_[ int(pos*nrBands()+1) ] < 128; }

    const TypeSet<float>&	histogram() const	{ return histogram_; }
				//!<nrSteps() values, the largest one is 1

private:

    ColTab::Mapper		mapper_;
    const float*		data_		= nullptr;
    od_int64			datasz_		= 0;
    mutable TypeSet<unsigned char> indices_;
    TypeSet<float>		histogram_;
};

} // namespace dGBVis
