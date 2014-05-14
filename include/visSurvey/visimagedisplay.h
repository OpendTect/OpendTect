#ifndef visimagedisplay_h
#define visimagedisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vislocationdisplay.h"
#include "visimagerect.h"

namespace visSurvey
{

/*!\brief
  Image
*/

mExpClass(visSurvey) ImageDisplay : public visSurvey::LocationDisplay
{
public:
    static ImageDisplay*	create()
    				mCreateDataObj(ImageDisplay);

    bool			setFileName(const char*);
    const char*			getFileName() const;
    Notifier<ImageDisplay>	needFileName;

    void			setSet(Pick::Set*);

     void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

protected:

     visBase::VisualObject*	createLocation() const;
     void			setPosition(int,const Pick::Location&);
     void			removePosition(int idx);
     void			setImageDataFromFile(const char* fnm);

     void			dispChg(CallBacker*);

     bool			hasDirection() const { return false; }

    				~ImageDisplay();
    void			setScene(visSurvey::Scene*);
    void			updateCoords(CallBacker* = 0);
    virtual int			clickedMarkerIndex(
					const visBase::EventInfo& evi)const;
    
    const mVisTrans*			displaytransform_;
    BufferString			imagefnm_;
    visBase::ImageRect::ImageData	imagedata_;
    RefMan<visBase::DataObjectGroup>	group_;
};

} // namespace visSurvey

#endif
