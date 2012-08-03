#ifndef vishingeline_h
#define vishingeline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          4-11-2002
 RCS:           $Id: vishingeline.h,v 1.12 2012-08-03 13:01:28 cvskris Exp $
________________________________________________________________________

          
-*/

#include "vissurveymod.h"
#include "visobject.h"

#include "emposid.h"

class RowCol;

namespace visBase { class IndexedPolyLine3D; class Transformation; };
namespace EM { class EdgeLineSet; };

namespace visSurvey
{

mClass(visSurvey) EdgeLineSetDisplay : public visBase::VisualObjectImpl
{
public:
    static EdgeLineSetDisplay*	create()
				mCreateDataObj(EdgeLineSetDisplay);

    void			setRadius(float);
    float			getRadius() const;

    void			setConnect(bool);
    				/*!<Sets wether the first and last node in a 
				    line should be connected, given that they
				    are neighbors */
    bool			getConnect() const;
    				/*!<\returns true if the first and last node
				     in a line is connected, given that they
				     are neighbors */
    void			setShowDefault(bool);
    				/*!<Sets wether default edgesegments should be
				    shown */
    bool			getShowDefault() const;
    				/*!<\returns wether default edgesegments
				  should be shown */

    void			setEdgeLineSet(const EM::EdgeLineSet*);
    bool			setEdgeLineSet(int);
    const EM::EdgeLineSet*	getEdgeLineSet() const { return edgelineset; }
    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;


    int				getRightClickedLine() const
    				{ return rightclidedline; }
    int				getRightClickedSegment() const
    				{ return rightclidedsegment; }
    int				getRightClickedSegmentPos() const
    				{ return rightclidedsegmentpos; }

protected:
    				~EdgeLineSetDisplay();
    void			updateEdgeLineSetChangeCB(CallBacker*);
    void			triggerRightClick(const visBase::EventInfo*);
    const EM::EdgeLineSet*	edgelineset;
    ObjectSet<visBase::IndexedPolyLine3D>	polylines;
    ObjectSet<TypeSet<int> >			polylinesegments;
    ObjectSet<TypeSet<int> >			polylinesegmentpos;
    const mVisTrans*				transformation;
    bool			connect;
    bool			showdefault;

    int				rightclidedline;
    int				rightclidedsegment;
    int				rightclidedsegmentpos;
};


}; // namespace visSurvey

#endif

