#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
________________________________________________________________________


-*/

#include "vislocationdisplay.h"
#include "visimagerect.h"

namespace OD { class RGBImage; }
namespace visSurvey
{

/*!\brief
  Image display, owns the RGBImage and distributes it to the child objects
*/

mExpClass(visSurvey) ImageDisplay : public visSurvey::LocationDisplay
{
public:
    static ImageDisplay*	create()
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

     visBase::VisualObject*	createLocation() const ;
     void			setPosition(int,const Pick::Location&,
					    bool add=false) override;
     void			removePosition(int idx) override;
     void			setImageDataFromFile(const char* fnm);

     void			dispChg(CallBacker*) override;

     bool			hasDirection() const override { return false; }

				~ImageDisplay();
    void			setScene(visSurvey::Scene*) override;
    void			updateCoords(CallBacker* =nullptr);
    virtual int			clickedMarkerIndex(
				    const visBase::EventInfo&) const override;

    const mVisTrans*			displaytransform_;
    BufferString			imagefnm_;
    const OD::RGBImage*			rgbimage_;
    RefMan<visBase::DataObjectGroup>	group_;
};

} // namespace visSurvey

