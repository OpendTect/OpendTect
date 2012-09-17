#ifndef binidsurface_h
#define binidsurface_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: binidsurface.h,v 1.18 2012/02/01 09:43:36 cvskris Exp $
________________________________________________________________________

-*/

#include "parametricsurface.h"

class BinIDValue;
class SurveyInfo;

template <class T> class Array2D;
template <class T> class TypeSet;

namespace Geometry
{


mClass BinIDSurface : public ParametricSurface
{
public:
    			BinIDSurface(const BinID& step);
    			BinIDSurface(const BinIDSurface&);
			~BinIDSurface();
    void		setInlCrlSystem(const SurveyInfo& si)
			{ surveyinfo_ = &si; }
    BinIDSurface*	clone() const;
    bool		isEmpty() const { return !depths_; }

    Coord3		computePosition(const Coord& param) const;

    void		setArray(const BinID& start,const BinID& step,
	    			 Array2D<float>*,bool takeover);
    			/*!<Mem is taken over by me if takeover is true. */
    Array2D<float>*	getArray()  { return depths_; }
    			/*Modify on your own responsibility.*/
    const Array2D<float>* getArray() const 
				    { return (const Array2D<float>*) depths_; }

    bool		insertRow(int row,int nrnew=1);
    bool		insertCol(int col,int nrnew=1);
    bool		removeRow(int,int);
    bool		removeCol(int,int);

    StepInterval<int>	colRange() const;
    StepInterval<int>	colRange(int row) const;

    bool		expandWithUdf(const BinID& start,const BinID& stop);

    virtual Coord3	getKnot( const RowCol& rc ) const
			{ return getKnot(rc,false); }
    Coord3		getKnot(const RowCol&,bool computeifudf) const;

    Coord		getKnotCoord(const RowCol&) const;

protected:
    void		_setKnot(int idx,const Coord3&);
    int			nrRows() const;
    int			nrCols() const;

    Array2D<float>*	depths_;
    const SurveyInfo*	surveyinfo_;
};

};

#endif
