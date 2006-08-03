#ifndef binidsurface_h
#define binidsurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: binidsurface.h,v 1.6 2006-08-03 13:29:14 cvskris Exp $
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

    Coord3		computePosition( const Coord& param ) const;

    void		setArray(const RCol& start,const RCol& step,
	    			 Array2D<float>*, bool takeover );
    			/*!<Mem is taken over by me if takeover is true. */
    Array2D<float>*	getArray() { return depths_; }
    			/*Modyfy on your own responsibility.*/

    bool		insertRow(int row);
    bool		insertCol(int col);
    bool		removeRow(int,int);
    bool		removeCol(int,int);

    Coord3		getKnot( const RCol&, bool computeifudf ) const;

protected:
    void		_setKnot( int idx, const Coord3& );
    int			nrRows() const;
    int			nrCols() const;

    Array2D<float>*	depths_;
};

};

#endif
