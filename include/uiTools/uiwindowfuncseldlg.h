

#include "uidialog.h"
#include "color.h"
#include "uiworld2ui.h"

class uiCanvas;

class uiWindowFuncSelDlg : public uiDialog
{

public:
			uiWindowFuncSelDlg(uiParent*);
			~uiWindowFuncSelDlg();

    void		setCurrentWindowFunc(const char*);

protected:
				
    void                reDraw(CallBacker*);

    void                setColor(const Color&);
    const Color&        getColor() const;
    uiCanvas*		canvas_;
    uiWorld2Ui          transform_;
    Color               color_;
    TypeSet<uiPoint>	pointlist_;

};
