#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Feb 2010
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "callback.h"
#include "trckeyzsampling.h"
#include "draw.h"
#include "emposid.h"
#include "flatview.h"
#include "randomlinegeom.h"

namespace FlatView { class Viewer; }
namespace Geometry { class FaultStickSurface; class ExplPlaneIntersection;
		     class PrimitiveSet; }

class FlatPosData;

namespace EM
{

class Fault3D;

/*!
\brief 3D fault painter.
*/

mExpClass(EarthModel) Fault3DPainter : public CallBacker
{
public:
    			Fault3DPainter(FlatView::Viewer&,const EM::ObjectID&);
			~Fault3DPainter();

    void		setTrcKeyZSampling(const TrcKeyZSampling&,bool);
    const TrcKeyZSampling& getTrcKeyZSampling() const	{ return tkzs_; }

    void		setPath(const TrcKeyPath&);
    void		setFlatPosData(const FlatPosData*);

    void		enableLine(bool);
    void		enableKnots(bool);

    void		setActiveStick(EM::PosID&);
    int			getActiveStickId() const      { return activestickid_; }
    void		setMarkerLineStyle(const OD::LineStyle&);
    bool		hasDiffActiveStick(const EM::PosID*) const;
    FlatView::AuxData*	getAuxData(const EM::PosID*) const;

	mStruct(EarthModel) StkMarkerInfo
	{
	    FlatView::AuxData*	marker_;
	    int			stickid_;
	};

    EM::FaultID&	getFaultID()			{ return emid_; }
    void		getDisplayedSticks(ObjectSet<StkMarkerInfo>&);

    Notifier<Fault3DPainter>	abouttorepaint_;
    Notifier<Fault3DPainter>	repaintdone_;

    void		paint();
    void		enablePaint(bool paintenable);

protected:
    bool		addPolyLine();

	mStruct(EarthModel) Fault3DMarker
	{
	    				Fault3DMarker(){}
					~Fault3DMarker()
					{
					    deepErase(stickmarker_);
					    deepErase(intsecmarker_);
					}
	    ObjectSet<StkMarkerInfo>		stickmarker_;
	    ObjectSet<FlatView::AuxData>	intsecmarker_;
	};

    bool		paintSticks(EM::Fault3D&,const EM::SectionID&,
				    Fault3DMarker*);
    bool		paintStickOnPlane(const Geometry::FaultStickSurface&,
	    				  RowCol&,const StepInterval<int>&,
					  const Coord3&, FlatView::AuxData&);
    bool		paintStickOnRLine(const Geometry::FaultStickSurface&,
	    				  RowCol&,const StepInterval<int>&,
					  const Coord3&, FlatView::AuxData&);
    bool		paintIntersection(EM::Fault3D&,const EM::SectionID&,
	    				  Fault3DMarker*);
    bool		paintPlaneIntxn(EM::Fault3D&,Fault3DMarker*,
	    				Geometry::ExplPlaneIntersection*,
					TypeSet<Coord3>&);
    void		genIntersectionAuxData(EM::Fault3D&,Fault3DMarker*,
	    				  const Geometry::PrimitiveSet* coordps,
					  TypeSet<Coord3>& intxnposs);
    void		removePolyLine();
    void		repaintFault3D();

    Coord		getNormalInRandLine( int idx ) const;
    FlatView::Point	getFVAuxPoint(const Coord3&) const;

    virtual void	fault3DChangedCB(CallBacker*);

    TrcKeyZSampling	tkzs_;
    const TrcKeyPath*	path_;
    const FlatPosData*  flatposdata_;
    TypeSet<int>	bendpts_;

    FlatView::Viewer&	viewer_;

    MarkerStyle2D	markerstyle_;

    EM::FaultID		emid_;
    ObjectSet<Fault3DMarker>    f3dmarkers_;

    int			activestickid_;
    bool		linenabled_;
    bool		knotenabled_;
    RandomLineID	rdlid_;
    bool		paintenable_;

public:
    void		setRandomLineID(RandomLineID rdlid);
};

} //namespace EM

