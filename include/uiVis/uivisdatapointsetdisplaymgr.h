#ifndef uivisdatapointsetdisplaymgr_h
#define uivisdatapointsetdisplaymgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2009
 RCS:           $Id: uivisdatapointsetdisplaymgr.h,v 1.6 2009-11-24 11:04:09 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "datapointset.h"
#include "menuhandler.h"
#include "thread.h"

class uiVisPartServer;


/*Implementation of DataPointSetDisplayMgr that enables display of
  datapointsets in 3d visualization scenes. */

mClass uiVisDataPointSetDisplayMgr : public DataPointSetDisplayMgr
{
public:
    		uiVisDataPointSetDisplayMgr(uiVisPartServer&);
		~uiVisDataPointSetDisplayMgr();

    void	lock();
    void	unLock();

    int		getNrParents() const;
    		//!<Will return the number of scenes
    const char*	getParentName(int) const;

    bool	hasDisplay() const			
    		{ return displayinfos_.size()>0; }
    int		hasDisplay( const DataPointSet&) const;			

    void	setDispCol(Color,int dispid);
    int		addDisplay(const TypeSet<int>& parents, const DataPointSet&);
    void	updateDisplay(int id, const TypeSet<int>& parents,
	    		      const DataPointSet&);
    void	removeDisplay(int);

    CNotifier<uiVisDataPointSetDisplayMgr,int>	treeToBeAdded;
    class DisplayInfo
    {
    public:
	TypeSet<int>		sceneids_;
	TypeSet<int>		visids_; //linked with scenes_
	int			dpsid_; //linked with scenes_
    };

protected:

    TypeSet<int>		allsceneids_;	

    MenuItem			createbodymnuitem_;
    MenuItem			storepsmnuitem_;
    MenuItem			removemnuitem_;
    MenuItem			sizemnuitem_;
    TypeSet<int>		ids_;	
    ObjectSet<DisplayInfo>	displayinfos_; //linked with ids_
    
    uiVisPartServer&		visserv_;
    MenuHandler*		vismenu_;
    Threads::Mutex		lock_;
    
    void	createMenuCB(CallBacker*);
    void	handleMenuCB(CallBacker*);
}; 
#endif
