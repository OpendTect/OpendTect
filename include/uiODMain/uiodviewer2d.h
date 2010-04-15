#ifndef uiodviewer2d_h
#define uiodviewer2d_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodviewer2d.h,v 1.15 2010-04-15 12:41:05 cvsbruno Exp $
________________________________________________________________________

-*/

#include "datapack.h"
#include "emposid.h"

class uiFlatViewAuxDataEditor;
class uiFlatViewStdControl;
class uiFlatViewWin;
class uiWellToSeisMainWin;
class uiMainWin;
class uiODMain;
class uiSlicePos2DView;
namespace Attrib { class SelSpec; }
namespace EM { class HorizonPainter; class uiEMViewer2DManager; }
namespace MPE { 
    		class HorizonFlatViewEditor;
	        class Fault3DFlatViewEditor; 	
		class FaultStickSetFlatViewEditor;
	      }


/*!\brief Manages the 2D Viewers
*/

mClass uiODViewer2D : public CallBacker
{
public:
				uiODViewer2D(uiODMain&,int visid);
				~uiODViewer2D();

    void			setUpView(DataPack::ID,bool wva);
    void			setSelSpec(const Attrib::SelSpec*,bool wva);

    uiFlatViewWin* 		viewwin() { return  viewwin_; }		
    uiFlatViewWin*		viewwin_;
    uiODMain&			appl_;

    int				visid_;
    BufferString		basetxt_;
    
protected:

    uiSlicePos2DView*				slicepos_;
    uiFlatViewStdControl*			viewstdcontrol_;
    ObjectSet<uiFlatViewAuxDataEditor>		auxdataeditors_;
    ObjectSet<MPE::HorizonFlatViewEditor> 	horfveditors_;
    ObjectSet<MPE::FaultStickSetFlatViewEditor> fssfveditors_;
    ObjectSet<MPE::Fault3DFlatViewEditor>	f3dfveditors_;
    ObjectSet<EM::uiEMViewer2DManager> 		emviewer2dmans_;

    Attrib::SelSpec&		wvaselspec_;
    Attrib::SelSpec&		vdselspec_;

    int				seltbid_;

    void			createViewWin(bool isvert);
    void			createViewWinEditors();
    void			winCloseCB(CallBacker*);
    void			posChg(CallBacker*);
    void			dataChangedCB(CallBacker*);

    void			updateOldActiveVolInUiMPEManCB(CallBacker*);
    void			restoreActiveVolInUiMPEManCB(CallBacker*);
    void			updateHorFlatViewerSeedPickStatus(CallBacker*);
};


mClass uiODWellSeisViewer2D : public uiODViewer2D
{
public:
				uiODWellSeisViewer2D(uiODMain&,int visid);

    bool			createViewWin(DataPack::ID,bool wva);
};

#endif
