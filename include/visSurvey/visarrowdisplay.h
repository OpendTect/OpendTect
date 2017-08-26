#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
________________________________________________________________________


-*/

#include "vissurveycommon.h"
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
    virtual void		setPosition(int idx,const Pick::Location&,
					    bool add) {};
    virtual void		removePosition(int);

    virtual int			clickedMarkerIndex(
					const visBase::EventInfo& evi)const;

    void			zScaleCB(CallBacker*);
    virtual void		dispChg();

    visBase::VisualObject*	createLocation() const;
    virtual bool		hasDirection() const { return true; }

    void			updateLineIndices(visBase::Lines*) const;

    Type			arrowtype_;
    visBase::DrawStyle*		linestyle_;
    const mVisTrans*		displaytransform_;
    RefMan<visBase::DataObjectGroup>	group_;
};


} // namespace visSurvey
