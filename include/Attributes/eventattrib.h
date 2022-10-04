#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"
#include "valseriesevent.h"

namespace Attrib
{

/*!
\brief %Event attribute
  
  EventAttrib  singleevent = 
	       eventtype = extremum, max, min, zerocrossing...
	       tonext =
	       gate =
	       outamp =
	       
  Calculates properties of events ( peakedness, steepness, asymmetry)
  Calculates the distance between the sample and the next or previous eventtype
  Calculates the distance between the sample and the sample of max or min 
  amplitude withing a time gate.
  Can optionally return the amplitude value at event exact position.
*/

mExpClass(Attributes) Event : public Provider
{
public:
    static void			initClass();

				Event(Desc&);
    
    static const char*		attribName()		{ return "Event"; }
    static const char*		eventTypeStr()		{ return "eventtype"; }
    static const char*		tonextStr()		{ return "tonext"; }
    static const char*		gateStr()		{ return "gate"; }
    static const char*		issingleeventStr()
				{ return "issingleevent"; }
    static const char*		outampStr()		{ return "outamp"; }


protected:
				~Event();
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			getInputOutput(int input,
					   TypeSet<int>& res) const override;
    bool			getInputData(const BinID&,int zintv) override;
    bool			computeData(const DataHolder&,
					    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const override;

    bool			allowParallelComputation() const override
				{ return true; }

    const Interval<float>*	reqZMargin(int input,int output) const override;
    const Interval<float>*	desZMargin(int input,int output) const override;

    
    static VSEvent::Type	getEventType(int type);
    ValueSeriesEvent<float,float> findNextEvent( 
					ValueSeriesEvent<float,float> nextev,
					int dir,VSEvent::Type,int,int) const;

    void			singleEvent(const DataHolder&,int,int) const;
    void			multipleEvents(const DataHolder&,int,int) const;

    const DataHolder*		inputdata_;

    bool			issingleevent_;
    bool			tonext_;
    VSEvent::Type		eventtype_;
    Interval<float>		gate_;
    int				dataidx_;
    bool			outamp_;
};

} // namespace Attrib
