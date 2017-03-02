#ifndef emhorizonpainter2d_h
#define emhorizonpainter2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id$
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
    void		updatePreferColors();


    Notifier<HorizonPainter2D>	abouttorepaint_;
    Notifier<HorizonPainter2D>	repaintdone_;

protected:

    bool		addPolyLine();
    void		removePolyLine();
    void		removeIntersectionMarkers();

    void		horChangeCB(CallBacker*);
    void		changePolyLineColor();
    void		updateIntersectionMarkers(int sid);
    Marker2D*		create2DMarker(const EM::SectionID&,float,float);
    bool		calcLine2DIntersections();


    EM::ObjectID	id_;
    TrcKeyZSampling	tkzs_;

    LineStyle		markerlinestyle_;
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
    Marker2D*		create2DMarker(const EM::SectionID&);
    void		updateSelectionColor();

};

} // namespace EM

#endif
