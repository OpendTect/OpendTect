#ifndef emfault3dpainter_h
#define emfault3dpainter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Feb 2010
 RCS:		$Id: emfault3dpainter.h,v 1.3 2010-04-21 07:46:17 cvsumesh Exp $
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
			~Fault3DPainter();

    void		setCubeSampling(const CubeSampling&,bool);
    const CubeSampling&	getCubeSampling() const			{ return cs_; }

    void		addFault3D(const MultiID&);
    void		addFault3D(const EM::ObjectID&);

    void		setActiveF3D(const EM::ObjectID&);
    void		setActiveStick(EM::PosID&);
    const int		getActiveStickId() const      { return activestickid_; }
    void		setMarkerLineStyle(const LineStyle&);
    bool		hasDiffActiveStick(const EM::PosID*) const;
    FlatView::Annotation::AuxData* getAuxData(const EM::PosID*) const;

	mStruct StkMarkerInfo
	{
	    FlatView::Annotation::AuxData*	marker_;
	    int					stickid_;
	};

    void		getDisplayedSticks(const EM::ObjectID&,
	    				   ObjectSet<StkMarkerInfo>&);

    Notifier<Fault3DPainter>	abouttorepaint_;
    Notifier<Fault3DPainter>	repaintdone_;

protected:
    bool		addPolyLine(const EM::ObjectID&);

	mStruct Fault3DMarker
	{
	    ObjectSet<StkMarkerInfo>			stickmarker_;
	    ObjectSet<FlatView::Annotation::AuxData>	intsecmarker_;
	};

    bool		paintSticks(EM::Fault3D*,const EM::SectionID&,
				    Fault3DMarker*);
    bool		paintIntersection(EM::Fault3D*,const EM::SectionID&,
	    				  Fault3DMarker*);
    void		removeFault3D(int);
    void		removePolyLine(int);
    void		repaintFault3D(const EM::ObjectID&);

    virtual void	fault3DChangedCB(CallBacker*);
     
    CubeSampling	cs_;
    FlatView::Viewer&	viewer_;

    LineStyle		markerlinestyle_;
    MarkerStyle2D	markerstyle_;

		mStruct Fault3DInfo
		{
		    EM::ObjectID        id_;
		    BufferString        name_;
		    bool                lineenabled_;
		    bool                nodeenabled_;
		};

    ObjectSet<Fault3DInfo>		    f3dinfos_;
    ObjectSet<ObjectSet<Fault3DMarker> >    f3dmarkers_;

    EM::ObjectID	activef3did_;
    int			activestickid_;
};

} //namespace EM

#endif
