#ifndef emhorizonpainter_h
#define emhorizonpainter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
 RCS:		$Id: emhorizonpainter.h,v 1.3 2009-06-02 10:35:22 cvsumesh Exp $
________________________________________________________________________

-*/

#include "callback.h"

#include "cubesampling.h"
#include "draw.h"
#include "emposid.h"
#include "flatview.h"
#include "multiid.h"
#include "position.h"


namespace EM
{

mClass HorizonPainter : public CallBacker
{
public:
    			HorizonPainter(FlatView::Viewer&);
			~HorizonPainter();

    void		addHorizon(const MultiID&);
    void                addHorizon(const EM::ObjectID&);
    void                setHorizonIDs(const ObjectSet<MultiID>*);
    void                removeHorizon(const MultiID&);

    void		setCubeSampling(const CubeSampling&,bool);

    void		setMarkerLineStyle(const LineStyle&);

    void		displayHorizon(const MultiID&,bool);
    bool		isDisplayed(const MultiID&) const;

    void		set2D(bool yn)		{ is2d_ = yn; }
    TypeSet<int>&	getTrcNos()		{ return trcnos_; }
    TypeSet<float>&	getDistances()		{ return distances_; }

protected:

    bool		addPolyLine(const EM::ObjectID&);
    void		changePolyLineColor(const EM::ObjectID&);
    void		changePolyLinePosition(const EM::ObjectID&,
	    				       const EM::PosID&);
    void		removeHorizon(int);
    void		removePolyLine(int);
    void		updateDisplay();
    void		repaintHorizon(const EM::ObjectID&);

    void		nrHorChangeCB(CallBacker*);
    virtual void	horChangeCB(CallBacker*);

    CubeSampling        cs_;
    Interval<float>     horrg_;

    LineStyle           markerlinestyle_;
    FlatView::Viewer&   viewer_;

    int			loadinghorcount_;
    EM::ObjectID	horidtoberepainted_;

    TypeSet<EM::ObjectID> horizonids_;
    ObjectSet< ObjectSet<ObjectSet<FlatView::Annotation::AuxData> > > 										hormarkerlines_;

    bool		is2d_;
    TypeSet<int>	trcnos_;
    TypeSet<float>	distances_;    
};

}// namespace EM

#endif
