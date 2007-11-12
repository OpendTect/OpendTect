#ifndef binidsurface_h
#define binidsurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: binidsurface.h,v 1.8 2007-11-12 16:07:43 cvsjaap Exp $
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
    			BinIDSurface(const RCol& step);
    			BinIDSurface(const BinIDSurface&);
			~BinIDSurface();
    BinIDSurface*	clone() const;

    Coord3		computePosition(const Coord& param) const;

    void		setArray(const RCol& start,const RCol& step,
	    			 Array2D<float>*,bool takeover);
    			/*!<Mem is taken over by me if takeover is true. */
    Array2D<float>*	getArray()  { return depths_; }
    			/*Modify on your own responsibility.*/
    const Array2D<float>* getArray() const 
				    { return (const Array2D<float>*) depths_; }

    bool		insertRow(int row);
    bool		insertCol(int col);
    bool		removeRow(int,int);
    bool		removeCol(int,int);

    bool		expandWithUdf(const RCol& start,const RCol& stop);

    Coord3		getKnot(const RCol&,bool computeifudf) const;

protected:
    void		_setKnot(int idx,const Coord3&);
    int			nrRows() const;
    int			nrCols() const;

    Array2D<float>*	depths_;
};

};

#endif
