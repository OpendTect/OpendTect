#ifndef basemapgrid_h
#define basemapgrid_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		October 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basemapmod.h"
#include "basemap.h"
#include "draw.h"
#include "multiid.h"
#include "posinfo2d.h"



namespace Basemap
{

mExpClass(Basemap) GridObject : public BaseMapObject
{
public:
				GridObject();
				~GridObject();

    const char*			getType() const		    { return "grid"; }
    void			updateGeometry();

    void			setInlCrlGrid(const StepInterval<double>& inlrg,
					      const StepInterval<double>& crlrg,
					      bool showinl,bool showcrl,
					      const LineStyle&);
    void			setXYGrid(const StepInterval<double>& xrg,
					const StepInterval<double>& yrg,
					const Geom::PosRectangle<double>& area,
					bool showx,bool showy,
					const LineStyle&);

    int				nrShapes() const;
    const char*			getShapeName(int shapeidx) const;
    void			getPoints(int shapeidx,TypeSet<Coord>&) const;
    const LineStyle*		getLineStyle(int shapeidx) const
				{ return &ls_; }

protected:
    LineStyle			ls_;

    StepInterval<double>	inlxgrid_;
    StepInterval<double>	crlygrid_;

    bool			isinlcrl_;
    int				nrinlx_;
    int				nrcrly_;
    Geom::PosRectangle<double>	xyarea_;

private:
    void			init(const StepInterval<double>&,
				     const StepInterval<double>&,
				     bool,bool,const LineStyle&);
};

} // namespace Basemap

#endif
