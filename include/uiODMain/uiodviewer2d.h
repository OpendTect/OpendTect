#ifndef uiodviewer2d_h
#define uiodviewer2d_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodviewer2d.h,v 1.3 2009-06-11 11:00:08 cvsnanne Exp $
________________________________________________________________________

-*/

#include "datapack.h"
#include "emposid.h"

class uiFlatViewAuxDataEditor;
class uiFlatViewWin;
class uiODMain;
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

    void			setUpView(DataPack::ID,bool wva,bool isvert);
    void			setSelSpec(const Attrib::SelSpec*);

    uiFlatViewWin*		viewwin_;
    uiODMain&			appl_;

    int				visid_;
    BufferString		basetxt_;
    
protected:

    EM::HorizonPainter*		horpainter_;
    uiFlatViewAuxDataEditor*	auxdataeditor_;
    MPE::HorizonFlatViewEditor*	horfveditor_;

    Attrib::SelSpec&		selspec_;

    void			createViewWin(bool isvert);
    void			winCloseCB(CallBacker*);
    void			drawHorizons();
};

#endif
