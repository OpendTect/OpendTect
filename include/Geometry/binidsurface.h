#ifndef binidsurface_h
#define binidsurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: binidsurface.h,v 1.2 2005-03-10 11:45:18 cvskris Exp $
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
    			BinIDSurface( const RCol& step );
    			BinIDSurface( const BinIDSurface& );
			~BinIDSurface();
    BinIDSurface*	clone() const;

    bool		insertRow(int row);
    bool		insertCol(int col);
    bool		removeRow(int row);
    bool		removeCol(int col);

    Coord3		getKnot( const RCol&, bool computeifudf ) const;

protected:
    void		_setKnot( int idx, const Coord3& );
    int			nrRows() const;
    int			nrCols() const;

    Array2D<float>*	depths;
};

};

#endif
