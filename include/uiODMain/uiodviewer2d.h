#ifndef uiodviewer2d_h
#define uiodviewer2d_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodviewer2d.h,v 1.9 2009-12-23 04:30:29 cvsumesh Exp $
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
namespace EM { class HorizonPainter; class uiEMViewer2DManager; }
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
    uiFlatViewAuxDataEditor*	auxdataeditor_;
    uiFlatViewStdControl*	viewstdcontrol_;
    MPE::HorizonFlatViewEditor*	horfveditor_;
    EM::uiEMViewer2DManager*	emviewer2dman_;

    Attrib::SelSpec&		wvaselspec_;
    Attrib::SelSpec&		vdselspec_;

    int				seltbid_;

    void			createViewWin(bool isvert);
    void			winCloseCB(CallBacker*);
    void			posChg(CallBacker*);
    void			dataChangedCB(CallBacker*);
    //void			fvselModeChangedCB(CallBacker*);

    void			updateOldActiveVolInUiMPEManCB(CallBacker*);
    void			restoreActiveVolInUiMPEManCB(CallBacker*);
    void			updateHorFlatViewerSeedPickStatus(CallBacker*);
};

#endif
