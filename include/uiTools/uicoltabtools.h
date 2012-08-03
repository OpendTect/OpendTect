#ifndef uicoltabtools_h
#define uicoltabtools_h

#include "uitoolsmod.h"
#include "callback.h"
#include "uirgbarraycanvas.h"

class uiRGBArray;
class uiWorld2Ui;
namespace ColTab { class Sequence; }

mClass(uiTools) uiColorTableCanvas : public uiRGBArrayCanvas
{
public:
				uiColorTableCanvas(uiParent*,
						   const ColTab::Sequence&,
						   bool withalpha,
						   bool vertical);
				~uiColorTableCanvas();
    void			setRGB();

    void			setFlipped(bool yn);

    bool			handleLongTabletPress();

protected:

    bool			vertical_;
    const ColTab::Sequence&	ctseq_;
    uiRGBArray*			rgbarr_;
    bool			flipseq_;

    uiRGBArray&			mkRGBArr(bool wa);
};

#endif

