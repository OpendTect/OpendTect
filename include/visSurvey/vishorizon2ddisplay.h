#ifndef vishorizon2ddisplay_h
#define vishorizon2ddisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: vishorizon2ddisplay.h,v 1.15 2009-07-22 16:01:25 cvsbert Exp $
________________________________________________________________________


-*/


#include "emposid.h"
#include "multiid.h"
#include "visemobjdisplay.h"

namespace visBase { class IndexedPolyLine3D; class PointSet; }
namespace EM { class Horizon2D; }
class ZAxisTransform;

namespace visSurvey
{

class Seis2DDisplay;

mClass Horizon2DDisplay : public EMObjectDisplay
{
public:
    static Horizon2DDisplay*	create()
				mCreateDataObj(Horizon2DDisplay);
    void			setDisplayTransformation(mVisTrans*);

    virtual void		getMousePosInfo(const visBase::EventInfo&,
						Coord3&,
						BufferString& val,
					       	BufferString& info) const;
    void			setLineStyle(const LineStyle&);

    EM::SectionID		getSectionID(int visid) const;

    bool			setDataTransform(ZAxisTransform*);
    const ZAxisTransform*	getDataTransform() const;

protected:
    				~Horizon2DDisplay();
    void			removeSectionDisplay(const EM::SectionID&);
    bool			addSection(const EM::SectionID&,TaskRunner*);

    struct LineRanges
    {
	TypeSet<TypeSet<Interval<int> > >	trcrgs;
	TypeSet<TypeSet<Interval<float> > >	zrgs;
    };

    bool			withinRanges(const RowCol&,float z,
					     const LineRanges& ) const;	
    void			updateSection(int idx,const LineRanges* lr=0);
					      
    void			emChangeCB(CallBacker*);
    bool			setEMObject(const EM::ObjectID&,TaskRunner*);

    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj );
    void			updateLinesOnSections(
	    				const ObjectSet<const Seis2DDisplay>&);
    void			updateSeedsOnSections(
	    				const ObjectSet<const Seis2DDisplay>&);

    void			zAxisTransformChg(CallBacker*);
    ZAxisTransform*		zaxistransform_;

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    ObjectSet<visBase::IndexedPolyLine3D>	lines_;
    ObjectSet<visBase::PointSet>		points_;
    TypeSet<EM::SectionID>			sids_;
};


};

#endif
