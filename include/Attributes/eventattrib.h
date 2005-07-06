#ifndef event_h
#define event_h

/*+
  ______________________________________________________________________

  CopyRight:     (C) dGB Beheer B.V.
  Author:        Helene Payraudeau
  Date:          February 2005
  ______________________________________________________________________

-*/

static const char* rcsID = "$Id: eventattrib.h,v 1.3 2005-07-06 15:02:07 cvshelene Exp $";

#include "arrayndimpl.h"
#include "limits.h"
#include "attribprovider.h"
#include "runstat.h"
#include "valseries.h"
#include "mathfunc.h"
#include "valseriesevent.h"

/*!\brief Event attribute
  
EventAttrib  singleevent = 
	     eventtype = extremum, max, min, zerocrossing...
	     tonext = 
	     gate = 

Calculates properties of events ( peakedness, steepness, asymmetry)
Calculates the distance between the sample and the next or previous eventtype
Calculates the distance between the sample and the sample of max or min 
amplitude withing a time gate.

*/

namespace Attrib
{

class Event : public Provider
{
public:
    static void         initClass();

			Event( Desc& );

protected:
    static Provider*    createInstance( Desc& );
    static void         updateDesc( Desc& );

    static const char*  attribName() { return "Event"; }
    static const char*  eventTypeStr() { return "eventtype"; }
    static const char*  issingleeventStr() { return "issingleevent"; }
    static const char*  tonextStr() { return "tonext"; }
    static const char*  gateStr() { return "gate"; }

    static Provider*    internalCreate( Desc&, ObjectSet<Provider>& existing );

    bool                getInputOutput( int input, TypeSet<int>& res ) const;
    bool                getInputData( const BinID&, int idx );
    bool                computeData( const DataHolder&, const BinID& relpos,
				     int t0, int nrsamples ) const;

    const Interval<float>*           reqZMargin(int input, int output) const;

    
    VSEvent::Type       	     findEventType() const; 
    ValueSeriesEvent<float,float>    findNextEvent( 
	    				ValueSeriesEvent<float,float> nextev, 
					int dir, VSEvent::Type evtype, int );

    void                singleEvent( TypeSet<float>&, int, int );
    void                multipleEvents( TypeSet<float>&, int, int );

    const DataHolder*   inputdata;

    bool          	issingleevent;
    bool          	tonext;
    int           	eventtype;
    Interval<float>     gate;

};

}; // namespace Attrib

#endif

        
