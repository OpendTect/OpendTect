#ifndef horizon2dline_h
#define horizon2dline_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        K. Tingdahl
Date:          March 2006
RCS:           $Id: horizon2dline.h,v 1.1 2006-04-26 21:10:52 cvskris Exp $
________________________________________________________________________

-*/

#include "rowcolsurface.h"
#include "samplingdata.h"

class Plane3;
class RowCol;

namespace Geometry
{

/*!A curve that goes along a fixed set of x,y coordinates with a varying
   z. */

class Horizon2DLine : public RowColSurface
{
public:
    			Horizon2DLine();
    			Horizon2DLine(const TypeSet<Coord>&,int start,int step);
    			Horizon2DLine(const Horizon2DLine&);
    			~Horizon2DLine();

    Horizon2DLine*	clone() const;

    int			addRow(const TypeSet<Coord>&,int start,int step);
    			/*!<\returns id of new path. */
    void		removeRow(int);
    void		setRow(int,const TypeSet<Coord>&,int start,int step);

    StepInterval<int>	rowRange() const;
    StepInterval<int>	colRange(int) const;

    Coord3		getKnot(const RCol&) const;
    bool		setKnot(const RCol&,const Coord3&);
    			/*!<\note the x and y coord will be ignored. */
    bool		isKnotDefined(const RCol&) const;

protected:
    int			colIndex( int rowidx, int colid ) const;

    ObjectSet<TypeSet<Coord3> >	rows_;
    TypeSet<SamplingData<int> >	colsampling_;
    int				firstrow_;
};

};

#endif
