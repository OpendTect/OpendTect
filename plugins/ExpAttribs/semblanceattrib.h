#ifndef similarityattrib_h
#define similarityattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          07-01-2008
 RCS:           $Id: semblanceattrib.h,v 1.4 2011/04/28 11:30:53 cvsbert Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"

/*!\brief Semblance Attribute

Semblance gate= pos0= pos1= stepout=1,1 extension=[0|90|180|Cube]
	 [steering=Yes|No]


If steering is enabled, it is up to the user to make sure that the steering
goes to the same position as pos0 and pos1 respectively.

Input:
0	Data
1	Steering

Extension:      0       90/180          Cube
1               pos0    pos0
2               pos1    pos1
3                       pos0rot
4                       pos1rot

Output:
0       Semblance

*/

namespace Attrib
{

mClass Semblance : public Provider
{
public:
    static void			initClass();
				Semblance(Desc&);

    static const char*		attribName()	{ return "Semblance"; }
    static const char*		gateStr()	{ return "gate"; }
    static const char*		pos0Str()	{ return "pos0"; }
    static const char*		pos1Str()	{ return "pos1"; }
    static const char*		stepoutStr()	{ return "stepout"; }
    static const char*		steeringStr()	{ return "steering"; }
    static const char*		extensionStr()	{ return "extension"; }
    static const char*		extensionTypeStr(int);
    virtual void		initSteering()	{ stdPrepSteering(stepout_); }

    void			prepPriorToBoundsCalc();

protected:
    				~Semblance() {}
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const
				{ return true; }

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const;

    const BinID*		reqStepout(int input,int output) const;
    const Interval<float>*	reqZMargin(int input,int output) const;
    const Interval<float>*	desZMargin(int input,int output) const;
    bool			getTrcPos();

    BinID			pos0_;
    BinID			pos1_;
    BinID			stepout_;
    Interval<float>		gate_;
    int				extension_;
    TypeSet<BinID>		trcpos_;

    Interval<float>             desgate_;

    bool			dosteer_;
    TypeSet<int>		steerindexes_;
    int				dataidx_;

    ObjectSet<const DataHolder>	inputdata_;
    const DataHolder*		steeringdata_;
};

} // namespace Attrib


#endif
