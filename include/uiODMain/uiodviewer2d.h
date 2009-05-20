#ifndef uiodviewer2d_h
#define uiodviewer2d_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodviewer2d.h,v 1.1 2009-05-20 08:34:13 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiodapplmgr.h"

#include "datapack.h"
#include "emposid.h"

class uiFlatViewWin;
namespace EM { class HorizonPainter; }


/*!\brief Manages the 2D Viewers
*/

mClass uiODViewer2D
{
public:
			uiODViewer2D(uiODMain&,int visid);
			~uiODViewer2D();

    void		setUpView(DataPack::ID,bool wva,bool isvert);

    uiFlatViewWin*	viewwin_;
    uiODMain&		appl_;
    EM::HorizonPainter*	horpainter_;

    int			visid_;
    BufferString	basetxt_;
    
protected:

    void		createViewWin(bool isvert);
    void		drawHorizons();
};

#endif
