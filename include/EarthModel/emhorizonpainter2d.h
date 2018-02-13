#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "emcommon.h"
#include "trckeyzsampling.h"
#include "flatview.h"
#include "geom2dintersections.h"

namespace EM
{

/*!\brief 2D horizon painter */

mExpClass(EarthModel) HorizonPainter2D : public CallBacker
{
public:
			HorizonPainter2D(FlatView::Viewer&,const DBKey&);
			~HorizonPainter2D();

    void		setTrcKeyZSampling(const TrcKeyZSampling&,
					   bool upd=false);
    void		setGeomID(Pos::GeomID);

    void		enableLine(bool);
    void		enableSeed(bool);
    bool		seedEnable() const { return seedenabled_; }

    TypeSet<int>&	getTrcNos()			{ return trcnos_; }
    TypeSet<float>&	getDistances()			{ return distances_; }

    void		paint(CallBacker* =0);
    void		displayIntersection(bool yn);

	mStruct(EarthModel)	Marker2D
	{
				Marker2D()
				    : marker_(0)
				    , sectionid_(-1)
				{}
				~Marker2D()
				{ delete marker_; }

	    FlatView::AuxData*	marker_;
	    EM::SectionID	sectionid_;
	};

    void		getDisplayedHor(ObjectSet<Marker2D>&);
    void		displaySelections(const TypeSet<EM::PosID>&);
    void		removeSelections();
    void		updateSelectionColor();

    Notifier<HorizonPainter2D>	abouttorepaint_;
    Notifier<HorizonPainter2D>	repaintdone_;

protected:

    bool		addPolyLine();
    void		removePolyLine();
    void		removeIntersectionMarkers();

    void		horChangeCB(CallBacker*);
    void		changePolyLineColor();
    void		updateIntersectionMarkers();
    Marker2D*		create2DMarker(float,float);
    bool		calcLine2DIntersections();


    DBKey		id_;
    TrcKeyZSampling	tkzs_;

    OD::LineStyle		markerlinestyle_;
    OD::MarkerStyle2D	markerstyle_;
    FlatView::Viewer&	viewer_;

    Pos::GeomID	geomid_;
    TypeSet<int>	trcnos_;
    TypeSet<float>	distances_;

    typedef ObjectSet<Marker2D>	SectionMarker2DLine;
    ObjectSet<SectionMarker2DLine>	markerline_;
    Marker2D*				markerseeds_;
    ObjectSet<Marker2D>			intsectmarks_;
    Marker2D*				selectionpoints_;


    bool		linenabled_;
    bool		seedenabled_;
    Line2DInterSectionSet intsectset_;


private:
    Marker2D*		create2DMarker();

};

} // namespace EM
