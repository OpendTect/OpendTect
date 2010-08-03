#ifndef emhorizonpainter3d_h
#define emhorizonpainter3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
 RCS:		$Id: emhorizonpainter3d.h,v 1.2 2010-08-03 09:03:28 cvsumesh Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "emposid.h"
#include "flatview.h"

namespace EM
{

mClass HorizonPainter3D : public CallBacker
{
public:
    			HorizonPainter3D(FlatView::Viewer&,const EM::ObjectID&);
			~HorizonPainter3D();

    void		setCubeSampling(const CubeSampling&,bool upd=false);

    void		enableLine(bool);
    void		enableSeed(bool);

    void		paint();

    	mStruct Marker3D
	{
	    			Marker3D()
				    : marker_(0)
				    , sectionid_(-1)
	    			{}
				~Marker3D()
				{ delete marker_; }
	     
	    FlatView::Annotation::AuxData*	marker_;
	    EM::SectionID                       sectionid_;	    
	};

    void		getDisplayedHor(ObjectSet<Marker3D>&);

    Notifier<HorizonPainter3D>	abouttorepaint_;
    Notifier<HorizonPainter3D>	repaintdone_;

protected:

    bool		addPolyLine();
    void		removePolyLine();

    void		horChangeCB(CallBacker*);
    void		changePolyLineColor();
    void		changePolyLinePosition( const EM::PosID& pid );
    void		repaintHorizon();

    EM::ObjectID        id_;
    CubeSampling        cs_;

    LineStyle           markerlinestyle_;
    MarkerStyle2D       markerstyle_;
    FlatView::Viewer&   viewer_;

    typedef ObjectSet<Marker3D>         SectionMarker3DLine;
    ObjectSet<SectionMarker3DLine>      markerline_;
    Marker3D*                           markerseeds_;

    bool		linenabled_;
    bool		seedenabled_;
};

} //namespace EM

#endif
