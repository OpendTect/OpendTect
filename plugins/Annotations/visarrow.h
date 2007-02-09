#ifndef visarrow_h
#define visarrow_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id: visarrow.h,v 1.3 2007-02-09 20:55:44 cvskris Exp $
________________________________________________________________________


-*/

#include "vislocationdisplay.h"

namespace visBase { class IndexedPolyLine; class DrawStyle; };

namespace Annotations
{

/*!\brief
  Arrow
*/

class ArrowDisplay : public visSurvey::LocationDisplay
{
public:
    static ArrowDisplay*	create()
				mCreateDataObj(ArrowDisplay);

    void			setScene( visSurvey::Scene* );

    enum Type			{ Top, Bot, Double };
    void			setType(Type);
    Type			getType() const;

    void			setLineWidth( int );
    int				getLineWidth() const;

protected:
    				~ArrowDisplay();
    void			zScaleCB(CallBacker*);
    void			dispChg(CallBacker*);
    visBase::VisualObject*	createLocation() const;
    void		setPosition(int,const Pick::Location&);
    bool		hasDirection() const { return true; }

    void		updateLineShape(visBase::IndexedPolyLine*) const;
    int			isMarkerClick(const TypeSet<int>& path) const;

    Type		arrowtype_;
    visBase::DrawStyle*	linestyle_;
};


} // namespace visBase

#endif
