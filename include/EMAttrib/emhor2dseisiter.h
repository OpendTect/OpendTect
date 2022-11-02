#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "multiid.h"

class Seis2DDataSet;
namespace EM { class Horizon2D; class Horizon2DGeometry; }


namespace EM
{

/*!
\brief Horizon2D line iterator.
*/

mExpClass(EMAttrib) Hor2DSeisLineIterator
{
public:

			Hor2DSeisLineIterator(const Horizon2D&);
			Hor2DSeisLineIterator(const MultiID&);
    virtual		~Hor2DSeisLineIterator();

    bool		next();
    bool		isValid() const;
    void		reset();
    void		getLineSet();
    int			nrLines() const			{ return nrlines_; }
    int			nrLinesDone() const		{ return lineidx_+1; }
    const char*		lineName() const;
    int			lineSetIndex(const char* attrnm=0) const;
    const MultiID&	lineSetKey()			{ return curlsid_; }

    const Horizon2D*	horizon() const			{ return h2d_; }
    const Horizon2DGeometry& geometry() const		{ return *geom_; }
    Seis2DDataSet*	dataSet()			{ return dataset_; }
    const Seis2DDataSet* dataSet() const		{ return dataset_; }

protected:

    const Horizon2D*		h2d_;
    const Horizon2DGeometry*	geom_;
    int				lineidx_;
    const int			nrlines_;
    Seis2DDataSet*		dataset_;
    MultiID			curlsid_;

private:

    void		init(const Horizon2D*);

};

} // namespace EM
