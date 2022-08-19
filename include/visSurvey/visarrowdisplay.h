#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    void			setScene(visSurvey::Scene*) override;

    enum Type			{ Top, Bottom, Double };
    void			setType(Type);
    Type			getType() const;

    void			setLineWidth( int );
    int				getLineWidth() const;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

protected:

    virtual void		setPosition(int,const Pick::Location&,
					    bool add=false) override;
    virtual void		removePosition(int) override;

    int				clickedMarkerIndex(const visBase::EventInfo&)
								const override;

    void			zScaleCB(CallBacker*);
    void			dispChg(CallBacker*) override;

    visBase::VisualObject*	createLocation() const;
    bool			hasDirection() const override { return true; }

    void			updateLineIndices(visBase::Lines*) const;

    Type			arrowtype_;
    visBase::DrawStyle*		linestyle_;
    const mVisTrans*		displaytransform_;
    RefMan<visBase::DataObjectGroup>	group_;
};


} // namespace visSurvey
