#ifndef uiodviewer2d_h
#define uiodviewer2d_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodviewer2d.h,v 1.2 2009-05-29 04:37:13 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiodapplmgr.h"

#include "datapack.h"
#include "emposid.h"

class uiFlatViewAuxDataEditor;
class uiFlatViewWin;
namespace EM { class HorizonPainter; }
namespace MPE { class HorizonFlatViewEditor; }


/*!\brief Manages the 2D Viewers
*/

mClass uiODViewer2D
{
public:
				uiODViewer2D(uiODMain&,int visid);
				~uiODViewer2D();

    void			setUpView(DataPack::ID,bool wva,bool isvert);

    uiFlatViewWin*		viewwin_;
    uiODMain&			appl_;

    int				visid_;
    BufferString		basetxt_;
    
protected:

    EM::HorizonPainter*		horpainter_;
    uiFlatViewAuxDataEditor*	auxdataeditor_;
    MPE::HorizonFlatViewEditor*	horfveditor_;

    void			createViewWin(bool isvert);
    void			drawHorizons();
};

#endif
