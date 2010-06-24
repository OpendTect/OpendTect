#ifndef uiodviewer2d_h
#define uiodviewer2d_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodviewer2d.h,v 1.18 2010-06-24 11:28:54 cvsumesh Exp $
________________________________________________________________________

-*/

#include "datapack.h"
#include "emposid.h"

class uiFlatViewAuxDataEditor;
class uiFlatViewStdControl;
class uiFlatViewWin;
class uiMainWin;
class uiODMain;
class uiODVw2DTreeTop;
class uiSlicePos2DView;
class uiTreeFactorySet;
class Vw2DDataManager;

namespace Attrib { class SelSpec; }

/*!\brief Manages the 2D Viewers
*/

mClass uiODViewer2D : public CallBacker
{
public:
				uiODViewer2D(uiODMain&,int visid);
				~uiODViewer2D();

    void			setUpView(DataPack::ID,bool wva);
    void			setSelSpec(const Attrib::SelSpec*,bool wva);

    uiFlatViewWin* 		viewwin() 		{ return  viewwin_; }
    Vw2DDataManager*		dataMgr()		{ return datamgr_; }
    
    const ObjectSet<uiFlatViewAuxDataEditor>&	dataEditor()	
    				{ return auxdataeditors_; }

    Attrib::SelSpec&		selSpec( bool wva )
    				{ return wva ? wvaselspec_ : vdselspec_; }
    uiFlatViewWin*		viewwin_;
    uiODMain&			appl_;

    int				visid_;
    BufferString		basetxt_;
    
protected:

    uiSlicePos2DView*				slicepos_;
    uiFlatViewStdControl*			viewstdcontrol_;
    ObjectSet<uiFlatViewAuxDataEditor>		auxdataeditors_;

    Attrib::SelSpec&		wvaselspec_;
    Attrib::SelSpec&		vdselspec_;

    Vw2DDataManager*		datamgr_;
    uiTreeFactorySet*		tifs_;
    uiODVw2DTreeTop*		treetp_;

    int				seltbid_;

    void			createViewWin(bool isvert);
    virtual void		createTree(uiMainWin*);
    void			createViewWinEditors();
    void			winCloseCB(CallBacker*);
    void			posChg(CallBacker*);
};

#endif
