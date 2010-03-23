#ifndef uistratdisplay_h
#define uistratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uistratdisplay.h,v 1.2 2010-03-23 13:36:56 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uiaxishandler.h"
#include "menuhandler.h"

class uiMenuHandler;
class uiParent;
class uiPolygonItem;
class uiStratMgr;
class uiTextItem;
class LayerSequence;
class MouseEvent;

namespace Strat{ class UnitRef; class NodeUnitRef; class Level; }

mClass uiAnnotDisplay : public uiGraphicsView
{
public:
			uiAnnotDisplay(uiParent*);

    mStruct MarkerData
    {
			MarkerData( const char* nm, float pos )
			    : zpos_(pos)
			    , name_(nm)
			    , order_(0)		     
			    {}

	float 		zpos_;
	const char*	name_;
	Color 		col_;
	uiTextItem*	txtitm_;
	uiLineItem*	itm_;
	int		order_; //optional sublayer number  
    };

    mStruct AnnotData : public MarkerData
    {
			AnnotData(const char* nm,float zpostop,float zposbot)
			    : MarkerData(nm,zpostop) 
			    , zposbot_(zposbot)
			    {}

	float 		zposbot_;
	uiPolygonItem*	plitm_;
    };

    void		setZRange( StepInterval<float> rg ) 
    			{ zax_.setBounds(rg); draw(); }
    void		dataChanged();

protected:

    ObjectSet<MarkerData> mrkdatas_;  //Horizontal Lines
    ObjectSet<AnnotData> annotdatas_; //Filled Rectangles
    int			nrsubannots_;
    uiAxisHandler 	zax_;

    uiMenuHandler&      menu_;
    MenuItem            addannotmnuitem_;
    MenuItem            remannotmnuitem_;

    void		draw();
    void		drawAnnots();
    void		drawLevels();
    virtual void	gatherInfo();
    bool 		handleUserClick(const MouseEvent&);
    void		updateAxis(); 
    
    void                createMenuCB(CallBacker*);
    void                handleMenuCB(CallBacker*);
    void		init(CallBacker*);
    void		reSized(CallBacker*);
    void                usrClickCB(CallBacker*);
};


mClass uiStratDisplay : public uiAnnotDisplay
{
public:
    			uiStratDisplay(uiParent*);

protected: 

    uiStratMgr* 	uistratmgr_;

    void		addNode(const Strat::NodeUnitRef&,int);
    void		addLevels();
    virtual void	addUnitAnnot(const Strat::UnitRef&,int);
    void		gatherInfo();
    virtual Interval<float> getUnitPos(const Strat::Level&,const Strat::Level&);
};

#endif
