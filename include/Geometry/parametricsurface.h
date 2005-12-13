#ifndef parametricsurface_h
#define parametricsurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: parametricsurface.h,v 1.9 2005-12-13 16:06:06 cvskris Exp $
________________________________________________________________________

-*/

#include "rowcol.h"
#include "geomelement.h"

template <class T> class Array2D;

namespace Geometry
{

class ParametricCurve;

class ParametricSurface : public Element
{
public:
    			ParametricSurface(
					const RCol& origo=RowCol(0,0),
					const RCol& step=RowCol(1,1) );
    			~ParametricSurface();
    ParametricSurface*	clone() const = 0;
    void		getPosIDs( TypeSet<GeomPosID>&, bool=true ) const;

    virtual Coord3 	computePosition(const Coord&) const;
    virtual Coord3 	computeNormal(const Coord&) const;

    virtual bool	insertRow(int row) 			= 0;
    virtual bool	insertCol(int col) 			= 0;
    virtual bool	removeRow(int row) 			= 0;
    virtual bool	removeCol(int col) 			= 0;

    StepInterval<int>	rowRange() const;
    StepInterval<int>	colRange() const;
    int			nrKnots() const;
    RowCol		getKnotRowCol( int idx ) const;
    int			getKnotIndex(const RCol& rc) const;

    virtual ParametricCurve*
			createRowCurve( float row,
	    				const Interval<int>* colrange=0 ) const;
    virtual ParametricCurve*
			createColCurve( float col,
	    				const Interval<int>* rowrange=0 ) const;

    virtual bool	circularRows() const { return false; }
    virtual bool	circularCols() const { return false; }

    virtual bool	setKnot( const RCol&, const Coord3& );
    virtual bool	unsetKnot( const RCol& );
    virtual Coord3	getKnot( const RCol&, bool estifundef=false ) const = 0;
    virtual bool	isKnotDefined( const RCol& ) const;
    bool		hasSupport(const RCol&) const;

    Coord3		getPosition( int64 pid ) const;
    bool		setPosition( int64 pid, const Coord3&);
    bool		isDefined( int64 pid ) const;

    bool		isAtEdge( const RCol& ) const;

    bool		checkSupport(bool yn);
    			/*!<Specifies wether support should be checked */
    bool		checksSupport() const;
    			/*!<\returns wether support should be checked */
    bool		checkSelfIntersection(bool yn);
    			/*!<Specifies wether support should be checked */
    bool		checksSelfIntersection() const;
    			/*!<\returns wether support should be checked */
protected:
    virtual void	_setKnot( int idx, const Coord3& ) 		= 0;
    virtual bool	checkSelfIntersection( const RCol& ) const;

    int		rowIndex(int row) const { return (row-origo.row)/step.row; }
    int		colIndex(int col) const { return (col-origo.col)/step.col; }
    static int	rowDim() { return 0; }
    static int	colDim() { return 1; }
    virtual int	nrRows() const 						= 0;
    virtual int	nrCols() const 						= 0;
    bool	isAtSameEdge(const RCol&,const RCol&,TypeSet<RowCol>* =0) const;

    bool	checksupport;
    bool	checkselfintersection;

    RowCol	origo;
    RowCol	step;
};

};

#endif
