#ifndef prestackanglecomputer_h
#define prestackanglecomputer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id$
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "refcount.h"
#include "flatposdata.h"

class MultiID;
class TraceID;

namespace Vel { class FunctionSource; }

namespace PreStack
{
class Gather;

mClass(PreStackProcessing) AngleComputer
{ mRefCountImpl(AngleComputer);
public:
				AngleComputer();
	
    bool			setMultiID(const MultiID&);
    bool			isOK() const { return velsource_; }
    
    void			setOutputSampling(const FlatPosData&);
    Gather*			computeAngles(const TraceID&);
    
protected:
    
    FlatPosData			outputsampling_;
    Vel::FunctionSource*	velsource_;
};



}; //namespace

#endif


