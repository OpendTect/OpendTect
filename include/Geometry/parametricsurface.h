#pragma once

/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        K. Tingdahl
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "rowcol.h"
#include "rowcolsurface.h"


namespace Geometry
{

class ParametricCurve;

mExpClass(Geometry) ParametricSurface : public RowColSurface
{ mODTextTranslationClass(ParametricSurface);
public:
			ParametricSurface(const RowCol& origin=RowCol(0,0),
					const RowCol& step=RowCol(1,1) );
			~ParametricSurface();
    ParametricSurface*	clone() const override				= 0;

    virtual Coord3	computePosition(const Coord&) const;
    virtual Coord3	computeNormal(const Coord&) const;

    virtual bool	insertRow(int row,int nrnew=1)			= 0;
    virtual bool	insertCol(int col,int nrnew=1)			= 0;
    virtual bool	removeRow(int startrow,int stoprow)  { return false; }
    virtual bool	removeCol(int startcol,int stoprcol) { return false; }

    StepInterval<int>	rowRange() const override;
    virtual StepInterval<int>	rowRange(int col) const;
    StepInterval<int>	colRange(int row) const override;
    StepInterval<int>	colRange() const override;

    virtual ParametricCurve*
			createRowCurve( float row,
					const Interval<int>* colrange=0 ) const;
    virtual ParametricCurve*
			createColCurve( float col,
					const Interval<int>* rowrange=0 ) const;

    virtual bool	circularRows() const { return false; }
    virtual bool	circularCols() const { return false; }

    bool		setKnot(const RowCol&,const Coord3&) override;
    virtual bool	unsetKnot(const RowCol&);
    Coord3		getKnot(const RowCol&) const override;
    virtual Coord3	getKnot(const RowCol&,bool interpifudf) const	= 0;
    bool		isKnotDefined(const RowCol&) const override;
    bool		hasSupport(const RowCol&) const;

    int			nrKnots() const;
    RowCol		getKnotRowCol( int idx ) const;

    Coord3		getPosition( GeomPosID pid ) const override;
    bool		setPosition( GeomPosID pid, const Coord3&) override;
    bool		isDefined( GeomPosID pid ) const override;

    bool		isAtEdge(const RowCol&) const;

    bool		checkSupport(bool yn);
			/*!<Specifies wether support should be checked */
    bool		checksSupport() const;
			/*!<\returns wether support should be checked */
    bool		checkSelfIntersection(bool yn);
			/*!<Specifies wether support should be checked */
    bool		checksSelfIntersection() const;
			/*!<\returns wether support should be checked */

    void		trimUndefParts() override;

    int			getKnotIndex(const RowCol& rc) const;
protected:

    virtual void	_setKnot( int idx, const Coord3& )		= 0;
    virtual bool	checkSelfIntersection( const RowCol& ) const;

    int rowIndex(int row) const
    {
	return (row - origin_.row()) / step_.row();
    }
    int colIndex(int col) const
    {
	return (col - origin_.col()) / step_.col();
    }
    static int	rowDim() { return 0; }
    static int	colDim() { return 1; }
    virtual int nrRows() const						= 0;
    virtual int nrCols() const						= 0;
    bool	isAtSameEdge(const RowCol&,const RowCol&,
			     TypeSet<RowCol>* =0) const;

    bool	checksupport_;
    bool	checkselfintersection_;

    RowCol	origin_;
    RowCol	step_;
};

};

