#ifndef uicoltabtools_h
#define uicoltabtools_h

#include "callback.h"
#include "uirgbarraycanvas.h"

class uiRGBArray;
class uiWorld2Ui;
namespace ColTab { class Sequence; }

class uiColorTableCanvas : public uiRGBArrayCanvas
{
public:
				uiColorTableCanvas(uiParent*,
						   const ColTab::Sequence&,
						   bool withalpha,
						   bool vertical);
				~uiColorTableCanvas();
    void			setRGB();

protected:

    bool			vertical_;
    const ColTab::Sequence&	ctseq_;
    uiRGBArray*			rgbarr_;

    uiRGBArray&			mkRGBArr(bool wa);
};

#endif
