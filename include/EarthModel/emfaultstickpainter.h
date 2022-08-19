#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "callback.h"

#include "coord.h"
#include "draw.h"
#include "emposid.h"
#include "flatview.h"
#include "integerid.h"
#include "trckeyzsampling.h"

namespace FlatView { class Viewer; }

class FlatPosData;

namespace EM
{

/*!
\brief %Fault stick painter.
*/

mExpClass(EarthModel) FaultStickPainter : public CallBacker
{
public:
    			FaultStickPainter(FlatView::Viewer&,
					  const EM::ObjectID&);
			~FaultStickPainter();

    void		setTrcKeyZSampling(const TrcKeyZSampling&,bool);
    const TrcKeyZSampling& getTrcKeyZSampling() const	{ return tkzs_; }

    void		setPath(const TrcKeyPath&);
    void		setRandomLineID(RandomLineID rdlid);
    void		setFlatPosData(const FlatPosData*);
    void		enableLine(bool);
    void		enableKnots(bool);

    void		setActiveStick(const EM::PosID&);
    int			getActiveStickId()	{ return activestickid_; }
    void		setMarkerLineStyle(const OD::LineStyle&);
    bool		hasDiffActiveStick(const EM::PosID*);
    FlatView::AuxData*	getAuxData(const EM::PosID*);

	mStruct(EarthModel)	StkMarkerInfo
	{
				StkMarkerInfo()
					: marker_(0)
					, stickid_(-1)
				{}
				~StkMarkerInfo()
				{ delete marker_; }

	    FlatView::AuxData*	marker_;
	    int			stickid_;
	};

    EM::ObjectID&	getFaultSSID()			{ return emid_; }
    void		getDisplayedSticks(ObjectSet<StkMarkerInfo>&);

    void		set2D(bool yn)		{ is2d_ = yn; }
    bool		is2D()			{ return is2d_; }
    const char* 	getLineName() const;
    void		setGeomID( Pos::GeomID geomid )
			{ geomid_ = geomid; }
    Pos::GeomID 	getGeomID() const		{ return geomid_; }
    Coord		getNormalToTrace( int trcnr ) const;
    Coord		getNormalInRandLine( int idx ) const;
    			//<! idx of BinID in path_ of RandomLine

    Notifier<FaultStickPainter>	abouttorepaint_;
    Notifier<FaultStickPainter> repaintdone_;

    TypeSet<int>&	getTrcNos()			{ return trcnos_; }
    TypeSet<float>&	getDistances()			{ return distances_; }
    TypeSet<Coord>&	getCoords()			{ return coords_; }

    void		paint();
    void		enablePaint(bool paintenable);

protected:

    bool		addPolyLine();

    bool		getNearestDistance(const Coord3& pos,float& dist);

    void		removePolyLine();
    void		repaintFSS();

    virtual void	fssChangedCB(CallBacker*);

    TrcKeyZSampling	tkzs_;
    const TrcKeyPath*	path_;
    const FlatPosData*	flatposdata_;

    MarkerStyle2D       markerstyle_;

    FlatView::Viewer&   viewer_;

    EM::ObjectID	emid_;

    ObjectSet<ObjectSet<StkMarkerInfo> >	sectionmarkerlines_;

    int			activestickid_;

    bool		is2d_;
    Pos::GeomID 	geomid_;

    TypeSet<int>	trcnos_;
    TypeSet<float>	distances_;
    TypeSet<Coord>	coords_;

    bool		linenabled_;
    bool		knotenabled_;
    bool		paintenable_;
    RandomLineID	rdlid_;
};

} // namespace EM
