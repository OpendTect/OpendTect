#ifndef parametricsurface_h
#define parametricsurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: parametricsurface.h,v 1.4 2005-02-17 10:25:47 cvskris Exp $
________________________________________________________________________

-*/

#include "rowcol.h"
#include "geomelement.h"

template <class T> class Array2D;

namespace Geometry
{

class ParametricSurface : public Element
{
public:
    			ParametricSurface(
					const RCol& origo=RowCol(0,0),
					const RCol& step=RowCol(1,1) );
    			~ParametricSurface();
    void		getPosIDs( TypeSet<GeomPosID>& ) const;

    virtual Coord3 	computePosition(const Coord&) const	= 0;
    virtual Coord3 	computeNormal(const Coord&) const	= 0;

    virtual bool	insertRow(int row) 			= 0;
    virtual bool	insertColumn(int col) 			= 0;
    virtual bool	removeRow(int row) 			= 0;
    virtual bool	removeColumn(int col) 			= 0;

    StepInterval<int>	rowRange() const;
    StepInterval<int>	colRange() const;

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
protected:
    virtual void	_setKnot( int idx, const Coord3& ) 		= 0;

    int		rowIndex(int row) const { return (row-origo.row)/step.row; }
    int		colIndex(int col) const { return (col-origo.col)/step.col; }
    int		getIndex(const RCol& rc) const;
    static int	rowDim() { return 0; }
    static int	colDim() { return 1; }
    virtual int	nrRows() const 						= 0;
    virtual int	nrCols() const 						= 0;
    bool	isAtSameEdge(const RCol&,const RCol&,TypeSet<RowCol>* =0) const;

    bool	checksupport;

    RowCol	origo;
    RowCol	step;
};

};

#endif
