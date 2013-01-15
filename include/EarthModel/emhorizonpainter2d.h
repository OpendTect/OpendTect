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
#include "earthmodelmod.h"
#include "cubesampling.h"
#include "emposid.h"
#include "flatview.h"

namespace EM
{

/*!
\ingroup EarthModel
\brief 2D horizon painter
*/

mClass(EarthModel) HorizonPainter2D : public CallBacker
{
public:
    			HorizonPainter2D(FlatView::Viewer&,const EM::ObjectID&);
			~HorizonPainter2D();

    void		setCubeSampling(const CubeSampling&,bool upd=false);
    void		setLineName(const char*);

    void		enableLine(bool);
    void		enableSeed(bool);

    TypeSet<int>&       getTrcNos()			{ return trcnos_; }
    TypeSet<float>&	getDistances()			{ return distances_; }

    void		paint();

    	mStruct(EarthModel)	Marker2D
	{
	    			Marker2D()
				    : marker_(0)
				    , sectionid_(-1)
				{}
				~Marker2D()
				{ delete marker_; }

	    FlatView::AuxData*  marker_;
	    EM::SectionID	sectionid_;
	};

    void		getDisplayedHor(ObjectSet<Marker2D>&);

    Notifier<HorizonPainter2D>	abouttorepaint_;
    Notifier<HorizonPainter2D>	repaintdone_;

protected:

    bool		addPolyLine();
    void		removePolyLine();

    void		horChangeCB(CallBacker*);
    void		changePolyLineColor();
    void		repaintHorizon();

    EM::ObjectID	id_;
    CubeSampling	cs_;

    LineStyle           markerlinestyle_;
    MarkerStyle2D       markerstyle_;
    FlatView::Viewer&   viewer_;

    const char*		linenm_;
    TypeSet<int>	trcnos_;
    TypeSet<float>	distances_;

    typedef ObjectSet<Marker2D> 	SectionMarker2DLine;
    ObjectSet<SectionMarker2DLine>	markerline_;
    Marker2D*				markerseeds_;

    bool		linenabled_;
    bool		seedenabled_;
};

}; //namespace EM


#endif


