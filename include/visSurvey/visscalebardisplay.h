#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Rannojay Sen
 Date:		March 2104
________________________________________________________________________


-*/

#include "vislocationdisplay.h"

namespace visSurvey
{

/*!\brief
  ScaleBar Display
*/

mExpClass(visSurvey) ScaleBarDisplay : public LocationDisplay
{
public:
    static ScaleBarDisplay*	create()
				mCreateDataObj(ScaleBarDisplay);
				~ScaleBarDisplay();

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

    visBase::VisualObject*	createLocation() const;

    virtual void		setPosition(int,const Pick::Location&);
    void			setPosition(int idx,const Pick::Location&,
					    bool add) override;

    void			removePosition(int) override;

    int				clickedMarkerIndex(const visBase::EventInfo&)
								const override;

    void			zScaleCB(CallBacker*);
    void			dispChg(CallBacker*) override;

    bool			oninlcrl_;
    int				orientation_;
    int				linewidth_;
    double			length_;
    const mVisTrans*		displaytransform_;
    RefMan<visBase::DataObjectGroup>	group_;
};


} // namespace visSurvey

