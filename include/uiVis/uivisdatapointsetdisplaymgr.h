#ifndef uivisdatapointsetdisplaymgr_h
#define uivisdatapointsetdisplaymgr_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Apr 2009
 RCS:           $Id: uivisdatapointsetdisplaymgr.h,v 1.1 2009-04-04 10:20:48 cvskris Exp $
________________________________________________________________________

-*/

#include "datapointset.h"
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

    int		addDisplay(const TypeSet<int>& parents, const DataPointSet&);
    void	updateDisplay(int id, const TypeSet<int>& parents,
	    		      const DataPointSet&);
    void	removeDisplay(int);

    class DisplayInfo
    {
    public:
	TypeSet<int>		sceneids_;
	TypeSet<int>		visids_; //linked with scenes_
    };

    TypeSet<int>		allsceneids_;	

    TypeSet<int>		ids_;	
    ObjectSet<DisplayInfo>	displayinfos_; //linked with ids_

    uiVisPartServer&		visserv_;
    Threads::Mutex		lock_;
}; 
#endif
