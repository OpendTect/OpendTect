#ifndef uiodviewer2d_h
#define uiodviewer2d_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "cubesampling.h"
#include "datapack.h"
#include "emposid.h"

class uiFlatViewAuxDataEditor;
class uiFlatViewStdControl;
class uiFlatViewWin;
class uiMainWin;
class uiODMain;
class uiODVw2DTreeTop;
class uiSlicePos2DView;
class uiToolBar;
class uiTreeFactorySet;
class MouseCursorExchange;
class Vw2DDataManager;

namespace Attrib { class SelSpec; }
namespace FlatView { class AuxData; }

/*!
\brief A 2D Viewer.
*/

mExpClass(uiODMain) uiODViewer2D : public CallBacker
{
public:
				uiODViewer2D(uiODMain&,int visid);
				~uiODViewer2D();

    virtual void		setUpView(DataPack::ID,bool wva);
    void			setSelSpec(const Attrib::SelSpec*,bool wva);
    void			setMouseCursorExchange(MouseCursorExchange*);

    uiFlatViewWin* 		viewwin() 		{ return  viewwin_; }
    const uiFlatViewWin* 	viewwin() const		{ return  viewwin_; }
    Vw2DDataManager*		dataMgr()		{ return datamgr_; }
    const Vw2DDataManager*	dataMgr() const		{ return datamgr_; }

    uiODVw2DTreeTop*		treeTop() 		{ return treetp_; }

    const uiTreeFactorySet*	uiTreeItemFactorySet() const { return tifs_; }
    
    const ObjectSet<uiFlatViewAuxDataEditor>&	dataEditor()	
    				{ return auxdataeditors_; }

    Attrib::SelSpec&		selSpec( bool wva )
    				{ return wva ? wvaselspec_ : vdselspec_; }
    const Attrib::SelSpec&	selSpec( bool wva ) const
    				{ return wva ? wvaselspec_ : vdselspec_; }

    void			setLineSetID( const MultiID& lsetid )
				{ linesetid_ = lsetid; }
    const MultiID&		lineSetID() const
				{ return linesetid_; }
    uiFlatViewStdControl*	viewControl() 
    				{ return viewstdcontrol_; }
    uiSlicePos2DView*		slicePos() 
    				{ return slicepos_; }
    uiODMain&			appl_;

    int				visid_;
    BufferString		basetxt_;

    virtual void		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;

    static const char*		sKeyVDSelSpec()  { return "VD SelSpec"; }
    static const char*		sKeyWVASelSpec() { return "WVA SelSpec"; }
    static const char*		sKeyPos() 	 { return "Position"; }

    Notifier<uiODViewer2D>	winClosed;

protected:

    uiSlicePos2DView*				slicepos_;
    uiFlatViewStdControl*			viewstdcontrol_;
    ObjectSet<uiFlatViewAuxDataEditor>		auxdataeditors_;

    Attrib::SelSpec&		wvaselspec_;
    Attrib::SelSpec&		vdselspec_;

    Vw2DDataManager*		datamgr_;
    uiTreeFactorySet*		tifs_;
    uiODVw2DTreeTop*		treetp_;
    uiFlatViewWin*		viewwin_;
    MouseCursorExchange*	mousecursorexchange_;
    FlatView::AuxData*		marker_;

    MultiID			linesetid_;
    CubeSampling		cs_;

    int				polyseltbid_;
    bool			ispolyselect_;

    virtual void		createViewWin(bool isvert);
    virtual void		createTree(uiMainWin*);
    virtual void		createPolygonSelBut(uiToolBar*);
    void			createViewWinEditors();
    virtual void		setPos(const CubeSampling&);
    void			adjustOthrDisp(bool wva,const CubeSampling&);

    void			rebuildTree();

    void			winCloseCB(CallBacker*);
    void			posChg(CallBacker*);
    void			selectionMode(CallBacker*);
    void			handleToolClick(CallBacker*);
    void			removeSelected(CallBacker*);
    void			mouseCursorCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
};

#endif

