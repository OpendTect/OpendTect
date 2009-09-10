#ifndef uiodviewer2d_h
#define uiodviewer2d_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodviewer2d.h,v 1.8 2009-09-10 11:11:49 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "datapack.h"
#include "emposid.h"

class uiFlatViewAuxDataEditor;
class uiFlatViewStdControl;
class uiFlatViewWin;
class uiODMain;
class uiSlicePos2DView;
namespace Attrib { class SelSpec; }
namespace EM { class HorizonPainter; }
namespace MPE { class HorizonFlatViewEditor; }


/*!\brief Manages the 2D Viewers
*/

mClass uiODViewer2D : public CallBacker
{
public:
				uiODViewer2D(uiODMain&,int visid);
				~uiODViewer2D();

    void			setUpView(DataPack::ID,bool wva);
    void			setSelSpec(const Attrib::SelSpec*,bool wva);

    uiFlatViewWin*		viewwin_;
    uiODMain&			appl_;

    int				visid_;
    BufferString		basetxt_;
    
protected:

    uiSlicePos2DView*		slicepos_;
    EM::HorizonPainter*		horpainter_;
    uiFlatViewAuxDataEditor*	auxdataeditor_;
    uiFlatViewStdControl*	viewstdcontrol_;
    MPE::HorizonFlatViewEditor*	horfveditor_;

    Attrib::SelSpec&		wvaselspec_;
    Attrib::SelSpec&		vdselspec_;

    int				seltbid_;

    void			createViewWin(bool isvert);
    void			winCloseCB(CallBacker*);
    void			posChg(CallBacker*);
    void			dataChangedCB(CallBacker*);
    void			fvselModeChangedCB(CallBacker*);

    void			updateOldActiveVolInUiMPEManCB(CallBacker*);
    void			restoreActiveVolInUiMPEManCB(CallBacker*);
    void			updateHorFlatViewerSeedPickStatus(CallBacker*);

    void			drawHorizons();
};

#endif
