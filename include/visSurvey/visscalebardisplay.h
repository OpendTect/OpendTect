#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vislocationdisplay.h"

#include "vistransform.h"

namespace visSurvey
{

/*!\brief
  ScaleBar Display
*/

mExpClass(visSurvey) ScaleBarDisplay : public LocationDisplay
{
public:
    static RefMan<ScaleBarDisplay> create();
				mCreateDataObj(ScaleBarDisplay);

    void			setOnInlCrl(bool);
    bool			isOnInlCrl() const;

    void			setOrientation(int);
    int				getOrientation() const;

    void			setLineWidth(int);
    int				getLineWidth() const;

    void			setLength(double);
    double			getLength() const;

    void			setColors(OD::Color);

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			fromPar(const IOPar&);
    void			toPar(IOPar&) const;

protected:
				~ScaleBarDisplay();

    RefMan<visBase::VisualObject> createLocation() const;

    virtual void		setPosition(int,const Pick::Location&);
    void			setPosition(int idx,const Pick::Location&,
					    bool add) override;

    void			removePosition(int) override;

    int				clickedMarkerIndex(const visBase::EventInfo&)
								const override;

    void			zScaleCB(CallBacker*);
    void			dispChg(CallBacker*) override;

    bool			oninlcrl_ = true;
    int				orientation_ = 0;
    int				linewidth_ = 2;
    double			length_ = 1000.;
    ConstRefMan<mVisTrans>	displaytransform_;
    RefMan<visBase::DataObjectGroup>	group_;
};


} // namespace visSurvey
