
#ifndef emfault3dpainter_h
#define emfault3dpainter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Feb 2010
 RCS:		$Id: emfault3dpainter.h,v 1.1 2010-02-12 08:41:31 cvsumesh Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "cubesampling.h"
#include "draw.h"
#include "emposid.h"
#include "flatview.h"

namespace FlatView { class Viewer; }

namespace EM
{

class Fault3D;

mClass Fault3DPainter : public CallBacker
{
public:
    			Fault3DPainter(FlatView::Viewer&);
			~Fault3DPainter() {}

	mStruct Fault3DInfo
	{
	    EM::ObjectID	id_;
	    BufferString	name_;
	    bool		lineenabled_;
	    bool		nodeenabled_;
	};

     void		addFault3D(const MultiID&);
     void		addFault3D(const EM::ObjectID&);

     void		setCubeSampling(const CubeSampling&,bool);

protected:
    bool		addPolyLine(const EM::ObjectID&);
    void		paintIntersection(EM::Fault3D*,const EM::SectionID&,
	    				  FlatView::Annotation::AuxData*);
     
    CubeSampling	cs_;
    FlatView::Viewer&	viewer_;

    LineStyle		markerlinestyle_;
    MarkerStyle2D	markerstyle_;

    ObjectSet<Fault3DInfo>	f3dinfos_;

    	mStruct Fault3DMarker
	{
	    ObjectSet<FlatView::Annotation::AuxData>*	stickmarker_;
	    FlatView::Annotation::AuxData*		intsecmarker_;
	};
    
    ObjectSet<ObjectSet<Fault3DMarker> >	f3dmarkers_;	
};

} //namespace EM


#endif
