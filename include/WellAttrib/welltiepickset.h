#ifndef welltiepickset_h
#define welltiepickset_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltiepickset.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "enums.h"
#include "namedobj.h"
#include "color.h"
#include "valseriesevent.h"
#include "welltieunitfactors.h"

namespace Well{ class Data; class LogSet; }
namespace WellTie
{
    class UserPick;
    class DataHolder;

mClass UserPick
{
    public:
			UserPick()
			{}

    Color           	color_;
    bool		issynthetic_;
    float           	zpos_;
    float           	xpos_;
};


mClass PickSet 
{
public:

			PickSet()
			    : mousepos_(0)
			    , nrpickstotal_(0)  
			    {}
			~PickSet();
			    
			PickSet( const PickSet& wps )
			    : mousepos_(wps.mousepos_)
			    , nrpickstotal_(wps.nrpickstotal_)
			    { 
				deepCopy(pickset_,wps.pickset_);
			    }


    void                add(UserPick* pick) { pickset_ += pick; };
    void                add(bool,float);
    void 		clear(int idx);
    void 		clearAll();
    const float         getMousePos() const    { return mousepos_; }
    WellTie::UserPick*  get(int idx)  	       { return pickset_[idx]; }
    const WellTie::UserPick* get(int idx) const { return pickset_[idx]; }
    WellTie::UserPick*  getLast()  	       { return pickset_[getSize()-1]; }
    const int           getTotalNr() const     { return nrpickstotal_; }
    const int           getSize() const        { return pickset_.size(); }
    float 		getPos( int idx )     
    			{ return get(idx)->zpos_; }
    const float 	getPos( int idx ) const 
    			{ return get(idx)->zpos_; }
    const float 	getLastPos() 		 
    			{ return getLast()->zpos_; }
   
    WellTie::UserPick*	remove(int idx) { return pickset_.remove(idx); }

    void         	setMousePos( float mp ) { mousepos_ = mp; }
    void		setPos( int idx, float pos ) 
    			{ get(idx)->zpos_= pos; }
    
    void                updateShift(int,float);

protected:

    float               mousepos_;
    int                 nrpickstotal_;
    ObjectSet<WellTie::UserPick> pickset_;
};


mClass PickSetMGR : public CallBacker
{
public:

			PickSetMGR(const DataHolder& dh)
			    : dholder_(dh)
			    , evtype_ (VSEvent::Extr)
			    , pickadded(this)
			    {}
    
    Notifier<PickSetMGR> pickadded;
    
    enum TrackType      { Maxima, Minima, ZeroCrossings };
			DeclareEnumUtils(TrackType)

    WellTie::PickSet* 	getSynthPickSet() { return &synthpickset_; }
    const WellTie::PickSet* getSynthPickSet() const { return &synthpickset_; } 
    WellTie::PickSet* 	getSeisPickSet()  { return &seispickset_; }
    const WellTie::PickSet* getSeisPickSet() const { return &seispickset_; }

    bool		lastpicksynth_;
    VSEvent::Type	evtype_;

    void           	addPick(float,bool);
    void           	clearAllPicks();
    void 	   	clearLastPicks();
    bool 	   	isPick();
    bool 	   	isSameSize();
    float 	   	findEvent(float,bool);
    void 	   	setData(WellTie::DataHolder*);
    void 	   	setEventType(int);
    void 	   	sortByPos(WellTie::PickSet&);
    void           	updateShift(int,float);


protected:

    const DataHolder& 	dholder_;
    const WellTie::Params::DataParams* datapms_;

    WellTie::PickSet 	synthpickset_;
    WellTie::PickSet 	seispickset_;
};

}; //namespace WellTie

#endif

