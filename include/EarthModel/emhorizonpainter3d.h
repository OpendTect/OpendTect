#ifndef emhorizonpainter3d_h
#define emhorizonpainter3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "trckeyzsampling.h"
#include "emposid.h"
#include "flatview.h"

class FlatPosData;

namespace EM
{

class Horizon3D;

/*!
\brief 3D horizon painter
*/

mExpClass(EarthModel) HorizonPainter3D : public CallBacker
{
public:
    			HorizonPainter3D(FlatView::Viewer&,const EM::ObjectID&);
			~HorizonPainter3D();

    void		setTrcKeyZSampling(
				const TrcKeyZSampling&,bool upd=false);
    void		setPath(const TrcKeyPath&);
    void		setFlatPosData(const FlatPosData*);

    void		enableLine(bool);
    void		enableSeed(bool);

    void		paint();

    	mStruct(EarthModel) Marker3D
	{
	    			Marker3D()
				    : marker_(0)
				    , sectionid_(-1)
	    			{}
				~Marker3D()
				{ delete marker_; }
	     
	    FlatView::AuxData*	marker_;
	    EM::SectionID	sectionid_;	    
	};

    void		getDisplayedHor(ObjectSet<Marker3D>&);

    Notifier<HorizonPainter3D>	abouttorepaint_;
    Notifier<HorizonPainter3D>	repaintdone_;

protected:

    typedef ObjectSet<Marker3D>         SectionMarker3DLine;

    bool		addPolyLine();
    void		removePolyLine();

    void		generateNewMarker(const Horizon3D&,const SectionID&,
	   				  SectionMarker3DLine&,Marker3D*&); 
    void		addDataToMarker(const BinID&,const Coord3&,
	    				const PosID&,const Horizon3D&,
					Marker3D&,int idx=-1);

    void		horChangeCB(CallBacker*);
    void		changePolyLineColor();
    void		changePolyLinePosition( const EM::PosID& pid );

    EM::ObjectID        id_;
    TrcKeyZSampling	tkzs_;
    const TrcKeyPath*	path_;
    const FlatPosData*	flatposdata_;

    LineStyle           markerlinestyle_;
    MarkerStyle2D       markerstyle_;
    FlatView::Viewer&   viewer_;

    ObjectSet<SectionMarker3DLine>      markerline_;
    Marker3D*                           markerseeds_;

    bool		linenabled_;
    bool		seedenabled_;
};

} //namespace EM

#endif


