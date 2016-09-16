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

    virtual void		setSet(Pick::Set*);

    virtual void		setDisplayTransformation(const mVisTrans*);
    virtual const mVisTrans*	getDisplayTransformation() const;

    void			setRGBImage(OD::RGBImage*);
				//!< Will become mine

protected:

     visBase::VisualObject*	createLocation() const;
     virtual void		setPosition(int,const Pick::Location&);
     virtual void		setPosition(int idx,const Pick::Location&,
					    bool add) {};

     virtual void		removePosition(int idx);
     void			setImageDataFromFile(const char* fnm);

     virtual void		dispChg();

     virtual bool		hasDirection() const { return false; }

				~ImageDisplay();
    void			setScene(visSurvey::Scene*);
    void			updateCoords(CallBacker* = 0);
    virtual int			clickedMarkerIndex(
					const visBase::EventInfo& evi)const;

    const mVisTrans*			displaytransform_;
    BufferString			imagefnm_;
    const OD::RGBImage*			rgbimage_;
    RefMan<visBase::DataObjectGroup>	group_;
};

} // namespace visSurvey
