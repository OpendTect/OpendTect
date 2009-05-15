#ifndef welltiepickset_h
#define welltiepickset_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltiepickset.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "namedobj.h"
#include "color.h"

class UserPick;

mClass UserPick
{
    public:
		    UserPick()
		    {}

    Color           color_;
    int             vidx_;
    float           zpos_;
};


mClass WellTiePickSetManager : public CallBacker
{
public:

			    WellTiePickSetManager();
                            ~WellTiePickSetManager();

    Notifier<WellTiePickSetManager> pickadded;
    Notifier<WellTiePickSetManager> mousemoving;

    const float         getMousePos() const   { return mousepos_; }
    
    const ObjectSet<UserPick>& getPickSet()  	{ return pickset_; }
    UserPick*           getPick( int idx )  	{ return pickset_[idx]; }
    UserPick*           getLastPick()  	{ return pickset_[pickSetSize()-1]; }

    const int           getPicksTotalNr() const { return nrpickstotal_; }
    const int           pickSetSize() const     { return pickset_.size(); }
    const float 	getPickPos( int idx ) 	{ return getPick(idx)->zpos_; }
    const float 	getLastPickPos() 	{ return getLastPick()->zpos_; }
    const int 		getLastPickVwrIdx()	{ return getLastPick()->vidx_; }

    void 		updateSupPickedPos(float&,float,int);
    void 		updateInfPickedPos(float&,float,int);

    void                addPick(int,float);
    void                updateShift(int,float);

    void 		clearPick(int idx);
    void 		clearAllPicks();


protected:

    ObjectSet<UserPick> pickset_;
    float               mousepos_;
    int                 nrpickstotal_;
};

#endif

