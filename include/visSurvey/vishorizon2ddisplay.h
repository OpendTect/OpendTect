#ifndef vishorizon2ddisplay_h
#define vishorizon2ddisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: vishorizon2ddisplay.h,v 1.6 2008-02-13 18:41:53 cvsjaap Exp $
________________________________________________________________________


-*/


#include "emposid.h"
#include "multiid.h"
#include "visemobjdisplay.h"

namespace visBase { class DrawStyle; class IndexedPolyLine; class PointSet; }
namespace EM { class Horizon2D; }

namespace visSurvey
{

class Seis2DDisplay;

class Horizon2DDisplay :  public  EMObjectDisplay
{
public:
    static Horizon2DDisplay*	create()
				mCreateDataObj(Horizon2DDisplay);
    void			setDisplayTransformation(mVisTrans*);

    EM::SectionID		getSectionID(int visid) const;

protected:
    				~Horizon2DDisplay();
    void			removeSectionDisplay(const EM::SectionID&);
    bool			addSection(const EM::SectionID&);

    void			updateSection(int idx,
				    const TypeSet<Interval<float> >* zrgs=0);
					      
    void			emChangeCB(CallBacker*);
    bool			setEMObject(const EM::ObjectID&);

    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj );
    void			updateLinesOnSections(
	    				const ObjectSet<const Seis2DDisplay>&);
    void			updateSeedsOnSections(
	    				const ObjectSet<const Seis2DDisplay>&);

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    ObjectSet<visBase::IndexedPolyLine>	lines_;
    ObjectSet<visBase::PointSet>	points_;
    TypeSet<EM::SectionID>		sids_;
};


};

#endif
