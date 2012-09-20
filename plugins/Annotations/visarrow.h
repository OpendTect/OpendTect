#ifndef visarrow_h
#define visarrow_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id$
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
    				~ArrowDisplay();

    void			setScene( visSurvey::Scene* );

    enum Type			{ Top, Bottom, Double };
    void			setType(Type);
    Type			getType() const;

    void			setLineWidth( int );
    int				getLineWidth() const;

protected:

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
