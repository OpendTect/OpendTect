#pragma once

/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "parametricsurface.h"

class BinIDValue;
class SurveyInfo;

template <class T> class Array2D;

namespace Geometry
{


mExpClass(Geometry) BinIDSurface : public ParametricSurface
{ mODTextTranslationClass(BinIDSurface);
public:
			BinIDSurface(const BinID& step);
			BinIDSurface(const BinIDSurface&);
			~BinIDSurface();
    BinIDSurface*	clone() const override;
    bool		isEmpty() const override { return !depths_; }

    Coord3		computePosition(const Coord& param) const override;
    Interval<float>	zRange();
    Interval<float>	zRange(Coord, Coord) const;
    Coord3		lineSegmentIntersection(Coord3, Coord3,
						float zshift=0.0);

    void		setArray(const BinID& start,const BinID& step,
				 Array2D<float>*,bool takeover);
			/*!<Mem is taken over by me if takeover is true. */
    Array2D<float>*	getArray()  { return depths_; }
			/*Modify on your own responsibility.*/
    const Array2D<float>* getArray() const
				    { return (const Array2D<float>*) depths_; }
    void		getPosIDs(TypeSet<GeomPosID>&,
				  bool rmudf=true) const override;

    bool		insertRow(int row,int nrnew=1) override;
    bool		insertCol(int col,int nrnew=1) override;
    bool		removeRow(int,int) override;
    bool		removeCol(int,int) override;

    StepInterval<int>	rowRange() const override;
    StepInterval<int>	rowRange(int col) const override;
    StepInterval<int>	colRange() const override;
    StepInterval<int>	colRange(int row) const override;

    bool		expandWithUdf(const BinID& start,const BinID& stop);

    virtual Coord3	getKnot( const RowCol& rc ) const override
			{ return getKnot(rc,false); }
    Coord3		getKnot(const RowCol&,bool computeifudf) const override;

    Coord		getKnotCoord(const RowCol&) const;
    RowCol		getNearestKnotRowCol(Coord) const;

protected:
    void		_setKnot(int idx,const Coord3&) override;
    int			nrRows() const override;
    int			nrCols() const override;

    Array2D<float>*	depths_;
    const SurveyInfo*	surveyinfo_;
    Interval<float>	zrange_;
};

} // namespace Geometry


