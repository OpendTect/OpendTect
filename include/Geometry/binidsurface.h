#ifndef binidsurface_h
#define binidsurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: binidsurface.h,v 1.1 2005-01-06 09:44:18 kristofer Exp $
________________________________________________________________________

-*/

#include "parametricsurface.h"

class BinIDValue;

template <class T> class Array2D;
template <class T> class TypeSet;

namespace Geometry
{


class BinIDSurface : public ParametricSurface
{
public:
    			BinIDSurface(const BinIDValue&, const BinIDValue&);
			~BinIDSurface();
    Coord3		computePosition(const Coord&) const;
    Coord3		computeNormal(const Coord&) const;

    bool		insertRow(int row);
    bool		insertColumn(int col);
    bool		removeRow(int row);
    bool		removeColumn(int col);

    Coord3		getKnot( const RCol& ) const;

protected:
    void		_setKnot( int idx, const Coord3& );
    int			nrRows() const;
    int			nrCols() const;

    Array2D<float>*	depths;
};

};

#endif
