#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vislocationdisplay.h"
#include "visimagerect.h"
#include "vistransform.h"

namespace OD { class RGBImage; }
namespace visSurvey
{

/*!\brief
  Image display, owns the RGBImage and distributes it to the child objects
*/

mExpClass(visSurvey) ImageDisplay : public LocationDisplay
{
public:
    static RefMan<ImageDisplay> create();
				mCreateDataObj(ImageDisplay);

    bool			setFileName(const char*);
    const char*			getFileName() const;
    Notifier<ImageDisplay>	needFileName;

    void			setSet(Pick::Set*) override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setRGBImage(OD::RGBImage*);
				//!< Will become mine

protected:
				~ImageDisplay();

     RefMan<visBase::VisualObject> createLocation() const;
     void			setPosition(int,const Pick::Location&,
					    bool add=false) override;
     void			removePosition(int idx) override;
     void			setImageDataFromFile(const char* fnm);

     void			dispChg(CallBacker*) override;

     bool			hasDirection() const override { return false; }

    void			setScene(Scene*) override;
    void			updateCoords(CallBacker* =nullptr);
    virtual int			clickedMarkerIndex(
				    const visBase::EventInfo&) const override;

    ConstRefMan<mVisTrans>		displaytransform_;
    BufferString			imagefnm_;
    const OD::RGBImage*			rgbimage_ = nullptr;
    RefMan<visBase::DataObjectGroup>	group_;
};

} // namespace visSurvey
