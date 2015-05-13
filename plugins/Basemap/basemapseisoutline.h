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
#include "coord.h"
#include "multiid.h"

template <class T> class ODPolygon;
class LineStyle;
namespace PosInfo {class CubeData;}
class TrcKeySampling;

namespace Basemap
{

mExpClass(Basemap) SegmentLine
{
public:
    Coord end1;
    Coord end2;

    bool	operator==(const SegmentLine& seg) const
    {
	return (end1==seg.end1 && end2==seg.end2);
    }
};


mExpClass(Basemap) SeisOutlineObject : public BaseMapObject
{
public:
			SeisOutlineObject();
			~SeisOutlineObject();
    bool		close(int) const;
    Alignment		getAlignment(int shapeidx) const;
    const LineStyle*	getLineStyle(int shapeidx) const    { return &ls_;}
    const MultiID&	getMultiID() const	{ return seismid_; }
    void		getPoints(int shapeidx,TypeSet<Coord>&) const;
    const char*		getShapeName(int shapeidx) const;
    const char*		getType() const		{ return "SeismicOutline"; }
    int			nrShapes() const;

    void		setFillColor(int idx,const Color&);
    const Color		getFillColor(int idx) const;

    void		setInsideLines(const StepInterval<int>&);
    virtual void	setLineStyle(int shapeidx,const LineStyle&);
    void		setMultiID(const MultiID&);

    bool		extractPolygons();
    bool		extractSegments();

    void		updateGeometry();

protected:
    bool		fullyrect_;
    Color		color_;
    int			nrsegments_;
    LineStyle&		ls_;
    MultiID		seismid_;
    ObjectSet< ODPolygon<float> >    polygons_;
    PosInfo::CubeData*	cubedata_;
    StepInterval<int>	linespacing_;
    TrcKeySampling&	seisarea_;
    TypeSet<SegmentLine> seglineset_;

private:
    bool		getCubeData();
};

} // namespace Basemap

#endif
