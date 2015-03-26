#ifndef basemapoutline_h
#define basemapoutline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		December 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basemapmod.h"
#include "basemap.h"
#include "multiid.h"

template <class T> class ODPolygon;
class LineStyle;
class TrcKeySampling;

namespace Basemap
{

mExpClass(Basemap) SeisOutlineObject : public BaseMapObject
{
public:
			SeisOutlineObject();
			~SeisOutlineObject();

    const MultiID&	getMultiID() const	{ return seismid_; }
    void		setMultiID(const MultiID&);
    const char*		getType() const     { return "SeismicOutline"; }
    void		updateGeometry();

    int			nrShapes() const;
    const char*		getShapeName(int shapeidx) const;
    void		getPoints(int shapeidx,TypeSet<Coord>&) const;
    const LineStyle*	getLineStyle(int shapeidx) const { return &ls_;}
    virtual void	setLineStyle(int shapeidx,const LineStyle&);

protected:
    bool		fullyrect_;
    LineStyle&		ls_;
    MultiID		seismid_;
    ObjectSet< ODPolygon<float> >    polygons_;
    TrcKeySampling&	seisarea_;

private:
    bool		extractPolygons();
};

} // namespace Basemap

#endif
