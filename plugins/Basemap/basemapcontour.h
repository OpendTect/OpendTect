#ifndef basemapcontour_h
#define basemapcontour_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		February 2015
 RCS:		$Id$
________________________________________________________________________

-*/


#include "basemapmod.h"
#include "basemap.h"
#include "multiid.h"

template <class T> class Array2DImpl;
template <class T> class ODPolygon;
class TaskRunner;
class TrcKeySampling;

namespace EM { class Horizon3D; }

namespace Basemap
{

mExpClass(Basemap) ContourObject : public BaseMapObject
{
public:
				ContourObject();
				~ContourObject();

    virtual const char*		getType() const     { return "contour"; }
    virtual void		updateGeometry();

    void			setLineStyle(int idx,const LineStyle&);
    void			setMultiID(const MultiID&,TaskRunner*);
    void			setContours(const StepInterval<float>& xrg,
					    TaskRunner*);

    virtual int			nrShapes() const;
    virtual const char*		getShapeName(int shapeidx) const;
    virtual void		getPoints(int shapeidx,TypeSet<Coord>&) const;
    virtual const LineStyle*	getLineStyle(int shapeidx) const
						    { return &ls_; }

protected:
    EM::Horizon3D*		hor3d_;
    LineStyle			ls_;
    MultiID			hormid_;
    TrcKeySampling&		horrange_;
    ObjectSet< ODPolygon<float> >    polygons_;
    TypeSet<float>		contourvals_;

private:
    bool		extractPolygons(const Array2DImpl<float>&,float);
};

} // namespace Basemap

#endif

