#ifndef flatauxdataeditor_h
#define flatauxdataeditor_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kris
 Date:          Mar 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "flatview.h"
#include "callback.h"
#include "geometry.h"
#include "keyenum.h"
#include "rcol2coord.h"

class MouseEventHandler;
class MouseEvent;
class MenuHandler;
class Color;

namespace FlatView
{

class AuxDataEditor;

#define mCtrlLeftButton ( (OD::ButtonState) (OD::LeftButton+OD::ControlButton) )

mExpClass(General) Sower : public CallBacker
{
    friend class	AuxDataEditor;

public:
    enum		SowingMode { Lasering=-2, Erasing=-1, Idle=0,
				     Furrowing, FirstSowing, SequentSowing };

    SowingMode		mode()				{ return mode_; }

    void		reInitSettings();

    void		reverseSowingOrder(bool yn=true);
    void		alternateSowingOrder(bool yn=true);
    void		intersow(bool yn=true);

    bool		moreToSow() const;
    void		stopSowing();

    Geom::Point2D<int>	pivotPos() const;

    bool		accept(const MouseEvent&,bool released=false);
    bool		activate(const Color&,const MouseEvent&);

    void		setSequentSowMask(bool yn=true,
				    OD::ButtonState mask=OD::LeftButton);
    void		setIfDragInvertMask(bool yn=true,
				    OD::ButtonState mask=OD::ShiftButton);
    void		setLaserMask(bool yn=true,
				    OD::ButtonState mask=OD::LeftButton);
    void		setEraserMask(bool yn=true,
				    OD::ButtonState mask=mCtrlLeftButton);

protected:
    			Sower(AuxDataEditor&,MouseEventHandler&);
			~Sower();

    void		setView(const Rect& wv,
	    			const Geom::Rectangle<int>& mouserect);

    bool		acceptMouse(const MouseEvent&,bool released);
    bool		acceptTablet(const MouseEvent&,bool released);
    bool		acceptLaser(const MouseEvent&,bool released);
    bool		acceptEraser(const MouseEvent&,bool released);

    void		reset();

    AuxDataEditor&		editor_;
    RCol2Coord			transformation_;
    AuxData*			sowingline_;
    MouseEventHandler&		mouseeventhandler_;
    Geom::PixRectangle<int>	mouserectangle_;
    SowingMode			mode_;
    ObjectSet<MouseEvent>	eventlist_;
    TypeSet<Coord>		mousecoords_;
    TypeSet<int>		bendpoints_;

    bool			reversesowingorder_;
    bool			alternatesowingorder_;
    bool			intersow_;

    OD::ButtonState		sequentsowmask_;
    OD::ButtonState		ifdraginvertmask_;
    OD::ButtonState		lasermask_;
    OD::ButtonState		erasermask_;

    bool			singleseeded_;
    int				curknotid_;
    int				curknotstamp_;
    int				furrowstamp_;
};



/*!Editor for FlatView::AuxData. Allows the enduser to
   click-drag-release the points in data.
   Users of the class have the choice if the editor should do the changes for
   them, or if they want to do changes themself, driven by the callback. */

mExpClass(General) AuxDataEditor : public CallBacker
{
    friend class	Sower;

public:
			AuxDataEditor(Viewer&,MouseEventHandler&);
    virtual		~AuxDataEditor();
    int			addAuxData(FlatView::AuxData*,bool doedit);
    			/*!<\param doedit says whether this object
			     should change the auxdata, or if the user
			     of the objects should do it.
			    \returns an id of the new set. */
    void		removeAuxData(int id);
    void		enableEdit(int id,bool allowadd,bool allowmove,
				   bool allowdelete);
    void		enablePolySel(int id,bool allowsel);
    void		setAddAuxData(int id);
    			//!<Added points will be added to this set.
    int			getAddAuxData() const;
    			/*!<\returns the id of the set that new points will
			     be added to */
    Notifier<AuxDataEditor>	addAuxDataChange;
    
    void		setView(const Rect& wv,
	    			const Geom::Rectangle<int>& mouserect);
    			/*!<User of the class must ensure that both
			    the wv and the mouserect are up to date at
			    all times. */
    const Geom::PixRectangle<int>& getMouseArea() const { return mousearea_; }
    Rect		getWorldRect(int dataid) const;
    void		limitMovement(const Rect*);
    			/*!<When movement starts, the movement is unlimited.
			    Movement can be limited once the movement started
			    by calling limitMovement. */
    bool		isDragging() const { return mousedown_; }

    int				getSelPtDataID() const;
    const TypeSet<int>&		getSelPtIdx() const;
    const Point&		getSelPtPos() const;
			
    Notifier<AuxDataEditor>	movementStarted;
    Notifier<AuxDataEditor>	movementFinished;
    				/*!
				  if getSelPtDataID==-1 selection polygon
				  			changed
				  else
				      If selPtIdx()==-1, position should be
				      			 added,
				      else		 point moved. */
    CNotifier<AuxDataEditor,bool> removeSelected;
    				/*!<Boolean is true if this is the end
				    of user interaction */

    void			setSelectionPolygonRectangle(bool);
    				//!<If not rectangle, it's a polygon
    bool			getSelectionPolygonRectangle() const;
    				//!<If not rectangle, it's a polygon
    const LineStyle&		getSelectionPolygonLineStyle() const;
    void			setSelectionPolygonLineStyle(const LineStyle&);
    void			getPointSelections(TypeSet<int>& ids,
	    					   TypeSet<int>& idxs) const;
    				/*!<Each point within the limits of the polygons
				    will be put in the typesets.*/

    const Viewer&	viewer() const	{ return viewer_; }
    Viewer&		viewer()	{ return viewer_; }

    Sower&		sower()		{ return *sower_; }

    void			setSelActive( bool yn ) { isselactive_ = yn; }
    bool			isSelActive() const	{ return isselactive_; }
    const TypeSet<int>&		getIds() const;
    const ObjectSet<AuxData>&	getAuxData() const;

    void		removePolygonSelected(int dataid);
    			//!<If dataid ==-1, all pts inside polygon is removed
    void		setMenuHandler(MenuHandler*);
    MenuHandler*	getMenuHandler();

    MouseEventHandler&	mouseEventHandler()	    { return mousehandler_; }

    const Point*	markerPosAt(const Geom::Point2D<int>& mousepos) const;

protected:
    void		getPointSelections( const ObjectSet<AuxData>& polygon,
			    TypeSet<int>& ids, TypeSet<int>& idxs) const;
			/*!<Each point within the limits of the polygons
			    will be put in the typesets.*/

    bool		removeSelectionPolygon();
    			//!<Returns true if viewer must be notified.
    void		mousePressCB(CallBacker*);
    void		mouseReleaseCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);

    void		findSelection(const Geom::Point2D<int>&,
				      int& seldatasetidx,
				      TypeSet<int>* selptidxlist) const;
    bool		updateSelection(const Geom::Point2D<int>&);
    			//!<\returns true if something is selected

    int			dataSetIdxAt(const Geom::Point2D<int>&) const;

    Viewer&			viewer_;
    Sower*			sower_;
    ObjectSet<AuxData>		auxdata_;
    TypeSet<int>		ids_;
    BoolTypeSet			allowadd_;
    BoolTypeSet			allowmove_;
    BoolTypeSet			allowremove_;
    BoolTypeSet			allowpolysel_;
    BoolTypeSet			doedit_;

    int				addauxdataid_;
    ObjectSet<AuxData>		polygonsel_;
    LineStyle			polygonsellst_;
    bool			polygonselrect_;
    bool			isselactive_;
    AuxData*			feedback_;
    Geom::Point2D<int>		prevpt_;

    Geom::PixRectangle<int>	mousearea_;
    Rect			curview_;
    MouseEventHandler&		mousehandler_;
    bool			mousedown_;
    bool			hasmoved_;

    int				seldatasetidx_;
    TypeSet<int>		selptidx_;
    Point			selptcoord_;
    Rect*			movementlimit_;

    MenuHandler*		menuhandler_;
};


}; // namespace FlatView

#endif

