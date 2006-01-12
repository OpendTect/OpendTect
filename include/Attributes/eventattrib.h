#ifndef eventattrib_h
#define eventattrib_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Helene Payraudeau
 Date:		February 2005
 RCS:		$Id: eventattrib.h,v 1.11 2006-01-12 13:15:30 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "valseriesevent.h"


namespace Attrib
{

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

class Event : public Provider
{
public:
    static void         initClass();

			Event(Desc&);
    
    static const char*  attribName() 		{ return "Event"; }
    static const char*  eventTypeStr() 		{ return "eventtype"; }
    static const char*  issingleeventStr() 	{ return "issingleevent"; }
    static const char*  tonextStr() 		{ return "tonext"; }
    static const char*  gateStr() 		{ return "gate"; }

protected:
    static Provider*    createInstance(Desc&);
    static void         updateDesc(Desc&);

    bool                getInputOutput(int input,TypeSet<int>& res) const;
    bool                getInputData(const BinID&,int idx);
    bool                computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples) const;

    bool		allowParallelComputation()	{ return true; }

    const Interval<float>*           reqZMargin(int input,int output) const;
    const Interval<float>*           desZMargin(int input,int output) const;

    
    static VSEvent::Type       	     getEventType(int type); 
    ValueSeriesEvent<float,float>    findNextEvent( 
	    				ValueSeriesEvent<float,float> nextev, 
					int dir,VSEvent::Type,int) const;

    void		singleEvent(TypeSet<float>&,int,int) const;
    void		multipleEvents(TypeSet<float>&,int,int) const;

    const DataHolder*	inputdata;

    bool		issingleevent;
    bool		tonext;
    VSEvent::Type	eventtype;
    Interval<float>	gate;
    int			dataidx_;

};

}; // namespace Attrib

#endif
