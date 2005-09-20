#ifndef shiftattrib_h
#define shiftattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: shiftattrib.h,v 1.4 2005-09-20 15:10:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "position.h"

/*!\brief Shift Attribute

  Shift pos= steering=Yes/No

  Shift takes the input at a specified position and outputs it at other
  relative positions.

Input:
0 - attrib to be hashed
1 - steering (optional)

Output
0 - hashed attrib

*/

namespace Attrib
{

class Shift : public Provider
{
public:
    static void		initClass();
			Shift(Desc&);

    static const char*	attribName()	{ return "Shift"; }
    static const char*	posStr()	{ return "pos"; }
    static const char*	timeStr()	{ return "time"; }
    static const char*	steeringStr()	{ return "steering"; }
    void		initSteering();

protected:
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);

    static Provider*	internalCreate(Desc&,ObjectSet<Provider>& existing);

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
	    			    int t0,int nrsamples) const;

    const BinID*		reqStepout(int input,int output) const;
    const Interval<float>*	desZMargin(int input,int output) const;

    BinID			pos;
    float 			time;
    bool			steering;
    
    BinID			stepout;
    Interval<float>		interval;
    int				dataidx_;

    const DataHolder*		inputdata;
    const DataHolder*		steeringdata;
};

}; // namespace Attrib


#endif
