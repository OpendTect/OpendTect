#ifndef visarrow_h
#define visarrow_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id: visarrow.h,v 1.1 2006-07-03 20:02:06 cvskris Exp $
________________________________________________________________________


-*/

#include "vislocationdisplay.h"

namespace visBase { class IndexedPolyLine; class DrawStyle; };

namespace visSurvey
{

/*!\brief
  Arrow
*/

class ArrowAnnotationDisplay : public LocationDisplay
{
public:
    static ArrowAnnotationDisplay*	create()
    					mCreateDataObj(ArrowAnnotationDisplay);

    void			setScene( visSurvey::Scene* );

    enum Type			{ Top, Bot, Double };
    void			setType(Type);
    Type			getType() const;

    void			setLineWidth( int );
    int				getLineWidth() const;

protected:
    				~ArrowAnnotationDisplay();
    void			zScaleCB(CallBacker*);
    void			dispChg(CallBacker*);
    visBase::VisualObject*	createLocation() const;
    void		setPosition(int,const Pick::Location&);
    bool		hasDirection() const { return true; }

    void		updateLineShape(visBase::IndexedPolyLine*) const;

    Type		arrowtype_;
    visBase::DrawStyle*	linestyle_;
};


} // namespace visBase

#endif
