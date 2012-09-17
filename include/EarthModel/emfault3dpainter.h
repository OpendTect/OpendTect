#ifndef emfault3dpainter_h
#define emfault3dpainter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Feb 2010
 RCS:		$Id: emfault3dpainter.h,v 1.5 2011/10/04 05:52:14 cvsumesh Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "cubesampling.h"
#include "draw.h"
#include "emposid.h"
#include "flatview.h"

namespace FlatView { class Viewer; }
namespace Geometry { class FaultStickSurface; class ExplPlaneIntersection; }

class FlatPosData;

namespace EM
{

class Fault3D;

mClass Fault3DPainter : public CallBacker
{
public:
    			Fault3DPainter(FlatView::Viewer&,const EM::ObjectID&);
			~Fault3DPainter();

    void		setCubeSampling(const CubeSampling&,bool);
    const CubeSampling&	getCubeSampling() const			{ return cs_; }
    void		setPath(const TypeSet<BinID>*);
    void		setFlatPosData(const FlatPosData*);

    void		enableLine(bool);
    void		enableKnots(bool);

    void		setActiveStick(EM::PosID&);
    const int		getActiveStickId() const      { return activestickid_; }
    void		setMarkerLineStyle(const LineStyle&);
    bool		hasDiffActiveStick(const EM::PosID*) const;
    FlatView::Annotation::AuxData* getAuxData(const EM::PosID*) const;

	mStruct StkMarkerInfo
	{
	    FlatView::Annotation::AuxData*	marker_;
	    int					stickid_;
	};

    EM::ObjectID&       getFaultID()			{ return emid_; }
    void		getDisplayedSticks(ObjectSet<StkMarkerInfo>&);

    Notifier<Fault3DPainter>	abouttorepaint_;
    Notifier<Fault3DPainter>	repaintdone_;

    void		paint();

protected:
    bool		addPolyLine();

	mStruct Fault3DMarker
	{
	    				Fault3DMarker(){}
					~Fault3DMarker()
					{
					    deepErase(stickmarker_);
					    deepErase(intsecmarker_);
					}
	    ObjectSet<StkMarkerInfo>			stickmarker_;
	    ObjectSet<FlatView::Annotation::AuxData>	intsecmarker_;
	};

    bool		paintSticks(EM::Fault3D&,const EM::SectionID&,
				    Fault3DMarker*);
    bool		paintStickOnPlane(const Geometry::FaultStickSurface&,
	    				  RowCol&,const StepInterval<int>&,
					  const Coord3&,
					  FlatView::Annotation::AuxData&);
    bool		paintStickOnRLine(const Geometry::FaultStickSurface&,
	    				  RowCol&,const StepInterval<int>&,
					  const Coord3&,
					  FlatView::Annotation::AuxData&);
    bool		paintIntersection(EM::Fault3D&,const EM::SectionID&,
	    				  Fault3DMarker*);
    bool		paintPlaneIntxn(EM::Fault3D&,Fault3DMarker*,
	    				Geometry::ExplPlaneIntersection*,
					TypeSet<Coord3>&);
    void		genIntersectionAuxData(EM::Fault3D&,Fault3DMarker*,
	    				       TypeSet<int>& coordindices,
					       TypeSet<Coord3>& intxnposs);
    void		removePolyLine();
    void		repaintFault3D();

    Coord		getNormalInRandLine( int idx ) const;

    virtual void	fault3DChangedCB(CallBacker*);
     
    CubeSampling	cs_;
    const TypeSet<BinID>* path_;
    const FlatPosData*  flatposdata_;
    TypeSet<int>	bendpts_;

    FlatView::Viewer&	viewer_;

    LineStyle		markerlinestyle_;
    MarkerStyle2D	markerstyle_;

    EM::ObjectID        emid_;
    ObjectSet<Fault3DMarker>    f3dmarkers_;

    int			activestickid_;
    bool		linenabled_;
    bool		knotenabled_;
};

} //namespace EM

#endif
