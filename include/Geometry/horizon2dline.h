#ifndef horizon2dline_h
#define horizon2dline_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        K. Tingdahl
Date:          March 2006
RCS:           $Id: horizon2dline.h,v 1.8 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "rowcolsurface.h"
#include "samplingdata.h"

class Plane3;
class RowCol;

namespace PosInfo { class Line2DData; }

namespace Geometry
{

/*!A curve that goes along a fixed set of x,y coordinates with a varying
   z. */

mClass Horizon2DLine : public RowColSurface
{
public:
    			Horizon2DLine();
    			Horizon2DLine(const TypeSet<Coord>&,int start,int step);
    			Horizon2DLine(const Horizon2DLine&);
    			~Horizon2DLine();

    Horizon2DLine*	clone() const;
    bool		isEmpty() const { return rows_.isEmpty(); }

    int			addRow(const TypeSet<Coord>&,int start,int step);
    			/*!<\returns id of new path. */
    int			addUdfRow(int start,int stop,int step);
    			/*!<\returns id of new path. */
    void		setRow(int,const TypeSet<Coord>&,int start,int step);
    void		syncRow(int,const PosInfo::Line2DData&);
    void		removeRow(int);
    void		removeCols(int rowid,int start,int stop);

    StepInterval<int>	rowRange() const;
    StepInterval<int>	colRange(int) const;
    Interval<float>	zRange(int) const;

    void		geometry(int rowid,PosInfo::Line2DData&) const;

    Coord3		getKnot(const RCol&) const;
    bool		setKnot(const RCol&,const Coord3&);
    bool		isKnotDefined(const RCol&) const;

    virtual void	trimUndefParts();
    bool		hasSupport(const RCol&) const;
    void		checkSupport( bool yn )	{ checksupport_ = yn; }
    bool		checksSupport() const	{ return checksupport_; }

protected:
    int			colIndex(int rowidx,int colid) const;

    bool		checksupport_;
    int			firstrow_;

    ObjectSet<TypeSet<Coord3> >	rows_;
    TypeSet<SamplingData<int> >	colsampling_;
};

} // namespace Geometry

#endif
