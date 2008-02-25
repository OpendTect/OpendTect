#ifndef vishorizon2ddisplay_h
#define vishorizon2ddisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: vishorizon2ddisplay.h,v 1.7 2008-02-25 16:12:47 cvsjaap Exp $
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

    struct LineRanges
    {
	TypeSet<TypeSet<Interval<int> > >	trcrgs;
	TypeSet<TypeSet<Interval<float> > >	zrgs;
    };

    bool			withinRanges(const RowCol&,float z,
					     const LineRanges& );	
    void			updateSection(int idx,const LineRanges* lr=0);
					      
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
