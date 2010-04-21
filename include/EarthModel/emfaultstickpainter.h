#ifndef emfaultstickpainter_h
#define emfaultstickpainter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id: emfaultstickpainter.h,v 1.6 2010-04-21 07:46:17 cvsumesh Exp $
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
			~FaultStickPainter();

    void		setCubeSampling(const CubeSampling&,bool);
    const CubeSampling&	getCubeSampling() const			{ return cs_; }

    void		addFaultStickSet(const MultiID&);
    void                addFaultStickSet(const EM::ObjectID&);

    void		setActiveFSS(const EM::ObjectID&);
    void		setActiveStick(EM::PosID&);
    int			getActiveStickId()	{ return activestickid_; }
    void		setMarkerLineStyle(const LineStyle&);
    bool		hasDiffActiveStick(const EM::PosID*);
    FlatView::Annotation::AuxData* getAuxData(const EM::PosID*);

    	mStruct StkMarkerInfo
	{
	    FlatView::Annotation::AuxData*	marker_;
	    int					stickid_;
	};

    void		getDisplayedSticks(const EM::ObjectID&,
	    				   ObjectSet<StkMarkerInfo>&);

    void		set2D(bool yn)		{ is2d_ = yn; }
    bool		is2D()			{ return is2d_; }
    void		setLineName( const char* ln ) 	{ linenm_ = ln; }
    const char*		getLineName() const		{ return linenm_; }
    void		setLineID( const MultiID& lsetid )	
    			{ lsetid_ = lsetid; }
    const MultiID&	getLineSetID() const		{ return lsetid_; }
    Coord		getNormalToTrace( int trcnr ) const;

    Notifier<FaultStickPainter>	abouttorepaint_;
    Notifier<FaultStickPainter> repaintdone_;

    TypeSet<int>&	getTrcNos()			{ return trcnos_; }
    TypeSet<float>&	getDistances()			{ return distances_; }
    TypeSet<Coord>&	getCoords()			{ return coords_; }

protected:

    bool		addPolyLine(const EM::ObjectID&);

    bool		getNearestDistance(const Coord3& pos,float& dist);

    void		removeFSS(int);
    void		removePolyLine(int);
    void		repaintFSS(const EM::ObjectID&);

    virtual void	fssChangedCB(CallBacker*);

    CubeSampling	cs_;

    LineStyle		markerlinestyle_;
    MarkerStyle2D       markerstyle_;

    FlatView::Viewer&   viewer_;

    		mStruct FaultStickSetInfo
	        {
		    EM::ObjectID        id_;
		    BufferString        name_;
		    bool                lineenabled_;
		    bool                nodeenabled_;
		};

    ObjectSet<FaultStickSetInfo>                        fssinfos_;

    ObjectSet<ObjectSet<ObjectSet<StkMarkerInfo> > >	faultmarkerline_;

    EM::ObjectID	activefssid_;
    int			activestickid_;

    bool		is2d_;
    const char*		linenm_;
    MultiID		lsetid_;
    
    TypeSet<int>	trcnos_;
    TypeSet<float>	distances_;
    TypeSet<Coord>	coords_;   
};

} //namespace EM

#endif
