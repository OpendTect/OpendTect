
#ifndef emfaultstickpainter_h
#define emfaultstickpainter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id: emfaultstickpainter.h,v 1.1 2010-01-21 10:02:33 cvsumesh Exp $
________________________________________________________________________

-*/

#include "callback.h"

#include "cubesampling.h"
#include "draw.h"
#include "emposid.h"
#include "flatview.h"
#include "position.h"

namespace FlatView { class Viewer; }

class MultiID;

namespace EM
{

mClass FaultStickPainter : public CallBacker
{
public:
    			FaultStickPainter(FlatView::Viewer&);
			~FaultStickPainter() {}

	mStruct FaultStickSetInfo
	{
	    EM::ObjectID	id_;
	    BufferString	name_;
	    bool		lineenabled_;
	    bool		nodeenabled_;
	};

    void		addFaultStickSet(const MultiID&);
    void                addFaultStickSet(const EM::ObjectID&);

    void		setCubeSampling(const CubeSampling&,bool);

    void		setMarkerLineStyle(const LineStyle&);

    void		set2D(bool yn)		{ is2d_ = yn; }
    void		setLineName(const char*);
    void		setLineID(MultiID*);
    TypeSet<int>&	getTrcNos()		{ return trcnos_; }
    TypeSet<float>&	getDistances()		{ return distances_; }
    TypeSet<Coord>&	getCoords()		{ return coords_; }

protected:

    bool		addPolyLine(const EM::ObjectID&);

    bool		getNearestDistance(const Coord3& pos,float& dist);

    CubeSampling	cs_;

    LineStyle		markerlinestyle_;
    MarkerStyle2D       markerstyle_;

    FlatView::Viewer&   viewer_;

    ObjectSet<FaultStickSetInfo>	fssinfos_;

    ObjectSet<ObjectSet<ObjectSet<FlatView::Annotation::AuxData> > >  
							faultmarkerline_;

    bool		is2d_;
    const char*		linenm_;
    const MultiID*	lineset_;
    
    TypeSet<int>	trcnos_;
    TypeSet<float>	distances_;
    TypeSet<Coord>	coords_;   
};

} //namespace EM

#endif
