#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2009
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "dbkey.h"
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
			Hor2DSeisLineIterator(const DBKey&);
    virtual		~Hor2DSeisLineIterator();

    bool		next();
    bool		isValid() const;
    void		reset();
    int			nrLines() const			{ return nrlines_; }
    int			nrLinesDone() const		{ return lineidx_+1; }
    const char*		lineName() const;
    const DBKey&	lineSetKey()			{ return curlsid_; }

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
    DBKey			curlsid_;

private:

    void		init(const Horizon2D*);

};

} // namespace EM
