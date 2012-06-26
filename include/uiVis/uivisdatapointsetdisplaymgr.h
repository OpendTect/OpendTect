#ifndef uivisdatapointsetdisplaymgr_h
#define uivisdatapointsetdisplaymgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2009
 RCS:           $Id: uivisdatapointsetdisplaymgr.h,v 1.11 2012-06-26 08:59:34 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "dpsdispmgr.h"
#include "menuhandler.h"
#include "thread.h"

class DataPointSet;
class uiVisPartServer;
namespace visSurvey { class PointSetDisplay; }


/*Implementation of DataPointSetDisplayMgr that enables display of
  datapointsets in 3d visualization scenes. */

mClass uiVisDataPointSetDisplayMgr : public DataPointSetDisplayMgr
{
public:
    		uiVisDataPointSetDisplayMgr(uiVisPartServer&);
		~uiVisDataPointSetDisplayMgr();

    void	lock();
    void	unLock();

    int		getNrViewers() const;
    		//!<Will return the number of scenes
    const char*	getViewerName(int) const;

    bool	hasDisplays() const			
    		{ return displayinfos_.size()>0; }
    DispID	getDisplayID( const DataPointSet&) const;			

    int		addDisplay(const TypeSet<int>& parents, const DataPointSet&);
    void	updateDisplay(DispID id, const TypeSet<int>& parents,
	    		      const DataPointSet&);
    void	updateDisplay(DispID id, const DataPointSet&);
    void	removeDisplay(DispID);
    void	clearDisplays();

    void	getIconInfo(BufferString& fnm,BufferString& tooltip) const;

    CNotifier<uiVisDataPointSetDisplayMgr,int>	treeToBeAdded;
    class DisplayInfo
    {
    public:
	TypeSet<int>		sceneids_;
	TypeSet<int>		visids_; //linked with scenes_
    };

protected:

    TypeSet<int>		allsceneids_;	

    MenuItem			createbodymnuitem_;
    MenuItem			storepsmnuitem_;
    MenuItem			removemnuitem_;
    MenuItem			propmenuitem_;
    TypeSet<int>		ids_;	
    ObjectSet<DisplayInfo>	displayinfos_; //linked with ids_
    
    uiVisPartServer&		visserv_;
    MenuHandler*		vismenu_;
    Threads::Mutex		lock_;
    
    void	createMenuCB(CallBacker*);
    void	handleMenuCB(CallBacker*);
};

#endif
