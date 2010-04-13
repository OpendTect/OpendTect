#ifndef uistratdispdata_h
#define uistratdispdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uistratdispdata.h,v 1.1 2010-04-13 12:55:16 cvsbruno Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "color.h"
#include "ranges.h"

//brief used to gather all units and tied levels from a tree for display

namespace Strat{ class UnitRef; class NodeUnitRef; class Level; }
class uiStratMgr;

mClass uiStratDisp : public CallBacker
{
public:
			uiStratDisp();
			~uiStratDisp();

    mStruct Level
    {
			Level( const char* nm, float pos )
			    : zpos_(pos)
			    , name_(nm)
			    , order_(0)		     
			    {}

	float 		zpos_;
	const char*	name_;
	Color 		col_;
	int		order_; //sublayer number  
    };

    mStruct Unit : public Level
    {
			Unit(const char* nm,float zpostop,float zposbot)
			    : Level(nm,zpostop) 
			    , zposbot_(zposbot)
			    {}

	float 		zposbot_;
	const char*	toplvlnm_;
	const char*	botlvlnm_;
    };
    
    ObjectSet<Level> 	levels_;  
    ObjectSet<Unit> 	units_; 

    void		gatherInfo();

    Notifier<uiStratDisp> dataChanged;

protected:

    const uiStratMgr&	uistratmgr_;

    void		addLevels();
    void		addUnits(const Strat::NodeUnitRef&,int);
    virtual void	addUnit(const Strat::UnitRef&,int);
    
    void		triggerDataChange(CallBacker*);
};

#endif
