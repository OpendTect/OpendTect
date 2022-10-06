#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "trckeyzsampling.h"
#include "emposid.h"
#include "flatview.h"
#include "geom2dintersections.h"


namespace EM
{

/*!
\brief 2D horizon painter
*/

mExpClass(EarthModel) HorizonPainter2D : public CallBacker
{
public:
			HorizonPainter2D(FlatView::Viewer&,const EM::ObjectID&);
			~HorizonPainter2D();

    void		setTrcKeyZSampling(const TrcKeyZSampling&,
					   bool upd=false);
    void		setGeomID(Pos::GeomID);

    void		enableLine(bool);
    void		enableSeed(bool);
    bool		seedEnable() const { return seedenabled_; }


    TypeSet<int>&	getTrcNos()			{ return trcnos_; }
    TypeSet<float>&	getDistances()			{ return distances_; }

    void		paint();
    void		displayIntersection(bool yn);

	mStruct(EarthModel) Marker2D
	{
				Marker2D();
				~Marker2D();

	    FlatView::AuxData*	marker_		= nullptr;
	};

    void		getDisplayedHor(ObjectSet<Marker2D>&);
    void		displaySelections(const TypeSet<EM::PosID>&);
    void		removeSelections();
    void		updatePreferColors();


    Notifier<HorizonPainter2D>	abouttorepaint_;
    Notifier<HorizonPainter2D>	repaintdone_;
    void		setLine2DInterSectionSet(const Line2DInterSectionSet*);
    const Line2DInterSectionSet* getLine2DInterSectionSet()
					    { return &intsectset_; }

protected:

    bool		addPolyLine();
    void		removePolyLine();
    void		removeIntersectionMarkers();

    void		horChangeCB(CallBacker*);
    void		changePolyLineColor();
    void		updateIntersectionMarkers();
    Marker2D*		create2DMarker(float x,float z);
    mDeprecated("Use create2DMarker without SectionID")
    Marker2D*		create2DMarker(const EM::SectionID&,float x,float z)
			{ return create2DMarker( x, z ); }
    bool		calcLine2DIntersections();


    EM::ObjectID	id_;
    TrcKeyZSampling	tkzs_;

    OD::LineStyle	markerlinestyle_;
    MarkerStyle2D	markerstyle_;
    FlatView::Viewer&	viewer_;

    Pos::GeomID 	geomid_;
    TypeSet<int>	trcnos_;
    TypeSet<float>	distances_;

    typedef ObjectSet<Marker2D> 	SectionMarker2DLine;
    ObjectSet<SectionMarker2DLine>	markerline_;
    Marker2D*				markerseeds_;

    bool		linenabled_;
    bool		seedenabled_;
    bool		intersection_;
    Line2DInterSectionSet intsectset_;
    ObjectSet<Marker2D>   intsectmarks_;
    Marker2D*		  selectionpoints_;

private:
    Marker2D*		create2DMarker();
    void		updateSelectionColor();

};

} // namespace EM
