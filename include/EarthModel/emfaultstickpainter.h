#ifndef emfaultstickpainter_h
#define emfaultstickpainter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "earthmodelmod.h"
#include "callback.h"

#include "cubesampling.h"
#include "draw.h"
#include "emposid.h"
#include "flatview.h"
#include "position.h"

namespace FlatView { class Viewer; }

class FlatPosData;
class MultiID;

namespace EM
{

/*!
\ingroup EarthModel
\brief %Fault stick painter.
*/

mExpClass(EarthModel) FaultStickPainter : public CallBacker
{
public:
    			FaultStickPainter(FlatView::Viewer&,
					  const EM::ObjectID&);
			~FaultStickPainter();

    void		setCubeSampling(const CubeSampling&,bool);
    const CubeSampling&	getCubeSampling() const			{ return cs_; }

    void                setPath(const TypeSet<BinID>*);
    void                setFlatPosData(const FlatPosData*);
    void		enableLine(bool);
    void		enableKnots(bool);

    void		setActiveStick(EM::PosID&);
    int			getActiveStickId()	{ return activestickid_; }
    void		setMarkerLineStyle(const LineStyle&);
    bool		hasDiffActiveStick(const EM::PosID*);
    FlatView::AuxData*	getAuxData(const EM::PosID*);

    	mStruct(EarthModel) StkMarkerInfo
	{
	    FlatView::AuxData*	marker_;
	    int			stickid_;
	};

    EM::ObjectID&	getFaultSSID()			{ return emid_; }
    void		getDisplayedSticks(ObjectSet<StkMarkerInfo>&);

    void		set2D(bool yn)		{ is2d_ = yn; }
    bool		is2D()			{ return is2d_; }
    void		setLineName( const char* ln ) 	{ linenm_ = ln; }
    const char*		getLineName() const		{ return linenm_; }
    void		setLineID( const MultiID& lsetid )	
    			{ lsetid_ = lsetid; }
    const MultiID&	getLineSetID() const		{ return lsetid_; }
    Coord		getNormalToTrace( int trcnr ) const;
    Coord		getNormalInRandLine( int idx ) const; 
    			//<! idx of BinID in path_ of RandomLine

    Notifier<FaultStickPainter>	abouttorepaint_;
    Notifier<FaultStickPainter> repaintdone_;

    TypeSet<int>&	getTrcNos()			{ return trcnos_; }
    TypeSet<float>&	getDistances()			{ return distances_; }
    TypeSet<Coord>&	getCoords()			{ return coords_; }

    void		paint();

protected:

    bool		addPolyLine();

    bool		getNearestDistance(const Coord3& pos,float& dist);

    void		removePolyLine();
    void		repaintFSS();

    virtual void	fssChangedCB(CallBacker*);

    CubeSampling	cs_;
    const TypeSet<BinID>*	path_;
    const FlatPosData*	flatposdata_;

    LineStyle		markerlinestyle_;
    MarkerStyle2D       markerstyle_;

    FlatView::Viewer&   viewer_;
    
    EM::ObjectID        emid_;

    ObjectSet<ObjectSet<StkMarkerInfo> >	sectionmarkerlines_;

    int			activestickid_;

    bool		is2d_;
    const char*		linenm_;
    MultiID		lsetid_;
    
    TypeSet<int>	trcnos_;
    TypeSet<float>	distances_;
    TypeSet<Coord>	coords_;

    bool		linenabled_;
    bool		knotenabled_;   
};

} //namespace EM

#endif


