#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vislocationdisplay.h"

#include "visdrawstyle.h"

namespace visBase { class Lines; };

namespace visSurvey
{

/*!\brief
  Arrow
*/

mExpClass(visSurvey) ArrowDisplay : public LocationDisplay
{
public:
    static RefMan<ArrowDisplay> create();
				mCreateDataObj(ArrowDisplay);

    void			setScene(Scene*) override;

    enum Type			{ Top, Bottom, Double };
    void			setType(Type);
    Type			getType() const;

    void			setLineWidth( int );
    int				getLineWidth() const;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

protected:
				~ArrowDisplay();

    void			setPosition(int,const Pick::Location&,
					    bool add=false) override;
    void			removePosition(int) override;

    int				clickedMarkerIndex(const visBase::EventInfo&)
								const override;

    void			zScaleCB(CallBacker*);
    void			dispChg(CallBacker*) override;

    RefMan<visBase::VisualObject> createLocation() const;
    bool			hasDirection() const override { return true; }

    void			updateLineIndices(visBase::Lines*) const;

    Type			arrowtype_	= Double;
    RefMan<visBase::DrawStyle>	linestyle_;
    ConstRefMan<mVisTrans>	displaytransform_;
    RefMan<visBase::DataObjectGroup>	group_;
};


} // namespace visSurvey
