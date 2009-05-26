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


mClass WellTiePickSet
{
public:

			    WellTiePickSet();
                            ~WellTiePickSet();

    const float         getMousePos() const   { return mousepos_; }
    void         	setMousePos( float mp ) { mousepos_ = mp; }
    
    void		setPos( int idx, float pos ) { get(idx)->zpos_=pos; }
    
    UserPick*           get(int idx)  	    { return pickset_[idx]; }
    const UserPick*     get(int idx) const  { return pickset_[idx]; }
    UserPick*           getLast()  	    { return pickset_[getSize()-1]; }
    const UserPick*     getLast(int idx) const { return pickset_[getSize()-1]; }

    const int           getTotalNr() const 	{ return nrpickstotal_; }
    const int           getSize() const     	{ return pickset_.size(); }
    float 		getPos( int idx ) 	{ return get(idx)->zpos_; }
    const float 	getPos( int idx ) const	{ return get(idx)->zpos_; }
    const float 	getLastPos() 		{ return getLast()->zpos_; }

    void 		updateSupPickedPos(float&,float,int);
    void 		updateInfPickedPos(float&,float,int);

    void                add(int,float);
    void                updateShift(int,float);

    void 		clear(int idx);
    void 		clearAll();

protected:

    ObjectSet<UserPick> pickset_;

    float               mousepos_;
    int                 nrpickstotal_;
};


mClass WellTiePickSetMGR : public CallBacker
{
public:

			    WellTiePickSetMGR();
                            ~WellTiePickSetMGR();

    CNotifier<WellTiePickSetMGR,int> pickadded;
    Notifier<WellTiePickSetMGR> mousemoving;

    WellTiePickSet* getLogPickSet()   		{ return &logpickset_; }
    WellTiePickSet* getSynthPickSet() 		{ return &synthpickset_; }
    WellTiePickSet* getSeisPickSet()  		{ return &seispickset_; }

    void           addPick(int,float);
    void           updateShift(int,float);
    void           clearAllPicks();

protected:

    WellTiePickSet logpickset_;
    WellTiePickSet synthpickset_;
    WellTiePickSet seispickset_;
};

#endif

