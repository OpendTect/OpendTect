#ifndef uistratdisplay_h
#define uistratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uistratdisplay.h,v 1.11 2010-05-11 14:22:11 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uigraphicsitem.h"
#include "uiaxishandler.h"
#include "uidialog.h"
#include "geometry.h"
#include "menuhandler.h"
#include "uistratdispdata.h"

class uiGenInput;
class uiGraphicsScene;
class uiLabeledSpinBox;
class uiMenuHandler;
class uiParent;
class uiPolygonItem;
class uiPolyLineItem;
class uiTextItem;
class MouseEvent;


mClass uiAnnotDrawer 
{
public:
				uiAnnotDrawer(uiGraphicsScene&,const AnnotData&);
				~uiAnnotDrawer();

    void			setZRange( StepInterval<float> rg ) 
    				{ yax_.setBounds(rg); draw(); }
    
    void			draw();
    
    uiAxisHandler& 		xAxis() 	{ return xax_; }
    uiAxisHandler& 		yAxis() 	{ return yax_; }
    const uiAxisHandler& 	xAxis() const	{ return xax_; }
    const uiAxisHandler& 	yAxis() const	{ return yax_; }

    mStruct ColumnItem
    {
				ColumnItem(const char* nm)
				    : name_(nm)
				    {}

	BufferString		name_;
	int			size_;
	
	uiPolyLineItem*		borderitm_;
	uiTextItem*		bordertxtitm_;
	ObjectSet<uiLineItem>	mrkitms_;
	ObjectSet<uiTextItem>	mrktxtitms_;
	ObjectSet<uiTextItem>	unittxtitms_;
	ObjectSet<uiPolygonItem> unititms_;
    };
    
    const ColumnItem&		colItem(int idx) const { return *colitms_[idx];}

protected:
    ObjectSet<ColumnItem>	colitms_;

    uiGraphicsScene&		scene_;
    uiAxisHandler 		yax_; 
    uiAxisHandler 		xax_; 

    //data
    const AnnotData&		data_;
    int 			nrCols() const { return data_.nrCols(); }
    
    //graphics
    void			addUnit(float);
    void			drawColumns();
    void			drawBorders(int);
    void			drawMarkers(int);
    void			drawUnits(int);
    void			eraseAll();
    void			updateAxis(); 
};


mClass uiAnnotDisplay : public uiGraphicsView
{
public:
				uiAnnotDisplay(uiParent*,const char*);
				~uiAnnotDisplay();

    const AnnotData&		data() const 	{ return data_; }

    void			setZRange( StepInterval<float> rg ) 
    				{ drawer_.setZRange( rg ); }
    int				nrUnits(int colidx) const 
    				{ return data_.getCol(colidx)->units_.size();}
    int				nrMarkers(int colidx) const 
    				{ return data_.getCol(colidx)->markers_.size();}
    AnnotData::Marker* 		getMarker(int idx, int colidx) 
    				{ return data_.getCol(colidx)->markers_[idx]; }
    AnnotData::Unit* 		getUnit(int idx,int colidx) 
    				{ return data_.getCol(colidx)->units_[idx]; }
    const AnnotData::Marker* 	getMarker(int idx,int colidx) const 
    				{ return data_.getCol(colidx)->markers_[idx]; }
    const AnnotData::Unit* 	getUnit(int idx,int colidx) const 
    				{ return data_.getCol(colidx)->units_[idx]; }
    int 			nrCols() const { return data_.nrCols(); }
    
    uiAxisHandler& 		xAxis() 	{ return drawer_.yAxis(); }
    uiAxisHandler& 		yAxis() 	{ return drawer_.xAxis(); }

protected:

    mClass uiCreateColDlg : public uiDialog
    {
	public :
				uiCreateColDlg(uiParent*);
	    BufferString        colname_;
	protected:
	    uiGenInput*         namefld_;
	    bool		acceptOK(CallBacker*);
    };

    bool                	handleUserClick(const MouseEvent&);
  
    uiMenuHandler&      	menu_;
    MenuItem            	addunitmnuitem_;
    MenuItem            	remunitmnuitem_;
    MenuItem            	addcolmnuitem_;

    //data
    AnnotData			data_;
    uiAnnotDrawer		drawer_;
    
    //position
    int				getColIdxFromPos() const; 
    AnnotData::Column*		getColFromPos() const; 
    const AnnotData::Unit* 	getUnitFromPos() const;
    Geom::Point2D<float> 	getPos() const;

    virtual void                createMenuCB(CallBacker*);
    virtual void                handleMenuCB(CallBacker*);
    void			init(CallBacker*);
    virtual void		reSized(CallBacker*);
    void                	usrClickCB(CallBacker*);
};


mClass uiStratDisplay : public uiAnnotDisplay
{
public:
				uiStratDisplay(uiParent*,uiStratRefTree&);
				~uiStratDisplay(){};
    
    void			display(bool,bool shrk=false,bool max=false);

protected :

    uiStratAnnotGather		uidatagather_;
    uiStratTreeWriter		uidatawriter_;

    MenuItem            	speclvlmnuitem_;
    MenuItem            	propunitmnuitem_;
    MenuItem            	addsubunitmnuitem_;
    
    uiGenInput*                 rangefld_;
    uiLabeledSpinBox*           stepfld_;
    uiGroup*			dispparamgrp_;
   
    void			createDispParamGrp();
    const AnnotData::Unit* 	getParentUnitFromPos() const;
    void			makeAnnots();
    void			makeAnnotCol(const char*,int,bool);

    void			dispParamChgd(CallBacker*);
    virtual void		dataChanged(CallBacker*);
    virtual void                handleMenuCB(CallBacker*);
    virtual void                createMenuCB(CallBacker*);
};


#endif
