#pragma once

#include "uitoolsmod.h"
#include "callback.h"
#include "uirgbarraycanvas.h"

class uiRGBArray;
class uiWorld2Ui;
namespace ColTab { class Sequence; }

mExpClass(uiTools) uiColorTableCanvas : public uiRGBArrayCanvas
{
public:
				uiColorTableCanvas(uiParent*,
						   const ColTab::Sequence&,
						   bool withalpha,
						   OD::Orientation);
				~uiColorTableCanvas();
    void			setRGB();

    void			setOrientation(OD::Orientation);
    void			setFlipped(bool yn);

    bool			handleLongTabletPress() override;

protected:

    OD::Orientation		orientation_;
    const ColTab::Sequence&	ctseq_;
    uiRGBArray*			rgbarr_;
    bool			flipseq_;

    uiRGBArray&			mkRGBArr(bool wa);
};

