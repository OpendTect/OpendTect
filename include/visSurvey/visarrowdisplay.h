#ifndef visarrowdisplay_h
#define visarrowdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vislocationdisplay.h"

namespace visBase { class Lines; class DrawStyle; };

namespace visSurvey
{

/*!\brief
  Arrow
*/

mExpClass(visSurvey) ArrowDisplay : public visSurvey::LocationDisplay
{
public:
    static ArrowDisplay*	create()
				mCreateDataObj(ArrowDisplay);
    				~ArrowDisplay();

    void			setScene(visSurvey::Scene*);

    enum Type			{ Top, Bottom, Double };
    void			setType(Type);
    Type			getType() const;

    void			setLineWidth( int );
    int				getLineWidth() const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

protected:

    virtual void		setPosition(int,const Pick::Location&);
    virtual void		removePosition(int);

    virtual int			clickedMarkerIndex(
					const visBase::EventInfo& evi)const;

    void			zScaleCB(CallBacker*);
    void			dispChg(CallBacker*);
    
    visBase::VisualObject*	createLocation() const;
    bool			hasDirection() const { return true; }

    void			updateLineIndices(visBase::Lines*) const;

    Type			arrowtype_;
    visBase::DrawStyle*		linestyle_;
    const mVisTrans*		displaytransform_;
    RefMan<visBase::DataObjectGroup>	group_;
};


} // namespace visSurvey

#endif
