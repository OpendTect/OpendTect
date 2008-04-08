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
				uiColorTableCanvas(uiParent*,ColTab::Sequence&,
						   bool);
				~uiColorTableCanvas();

    uiRGBArray&			mkRGBArr();
    const ColTab::Sequence&	getColSeq()          	{ return ctseq_; }
    void			setColTab(ColTab::Sequence&);
    void			reFill(CallBacker*);

    Notifier<uiColorTableCanvas> coltabchgd;

protected:

    bool			vertical_;
    ColTab::Sequence&		ctseq_;
    uiRGBArray*			rgbarr_;
};

#endif

