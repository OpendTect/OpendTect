#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgmod.h"

#include "uidialog.h"

#include "visvsgsurvscene.h"
#include "visvsgtexturechannels.h"

class uiColorTableGroup;

/*!\brief The color table and its range for the data of a VSG display,
  with the color table editor of OpendTect, as in uiODEditAttribColorDlg.

  It edits the plane of the test scene that was grabbed last. A change is
  applied to the display at once.
*/

mExpClass(uiVSG) uiVSGColTabDlg : public uiDialog
{ mODTextTranslationClass(uiVSGColTabDlg);
public:
			uiVSGColTabDlg(uiParent*,dGBVisSurvey::Scene&);
			~uiVSGColTabDlg();

private:

    void		seqChgCB(CallBacker*);
    void		scaleChgCB(CallBacker*);
    void		channelsCB(CallBacker*);
    void		bind();

    RefMan<dGBVisSurvey::Scene>		scene_;
    RefMan<dGBVis::TextureChannels>	channels_;
    uiColorTableGroup*			coltab_;
};
