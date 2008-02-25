#ifndef volprochorinterfiller_h
#define volprochorinterfiller_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id: volprochorinterfiller.h,v 1.1 2008-02-25 19:14:54 cvskris Exp $
________________________________________________________________________

-*/

#include "multiid.h"
#include "samplingdata.h"
#include "volprocchain.h"

class BinID;
namespace EM { class EMObject; class Horizon; }

namespace VolProc
{
    
class HorInterFiller : public Step
{
public:
    static void			initClass();
    
    				~HorInterFiller();
				HorInterFiller(Chain&);

    const char*			type() const { return sKeyType(); }
    bool			needsInput(const HorSampling& ) const
                                	{ return true; }			

    bool 			setTopHorizon(const MultiID*,float tv);
    const MultiID*		getTopHorizonID() const;
    float			getTopValue() const;
    
    bool			setBottomHorizon(const MultiID*,float bv);	
    const MultiID*		getBottomHorizonID() const;
    float			getBottomValue() const;
    
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    
    static const char*		sKeyType()	{ return "HorInterFiller"; }
    
protected:
    bool			prefersBinIDWise() const        { return true; }
    bool			computeBinID(const BinID&, int);
    bool                        prepareComp(int)		{ return true; }

    static const char*		sKeyTopHorID()	{ return "Top horizon"; }
    static const char*		sKeyBotHorID()	{ return "Bottom horizon"; }
    static const char*		sKeyTopValue()	{ return "Top Value"; }
    static const char*		sKeyBotValue()	{ return "Bottom Value"; }
  
    EM::Horizon*		loadHorizon(const MultiID&) const;
    static Step*      		create(Chain&);

    float    			topvalue_;    
    float			bottomvalue_;
    EM::Horizon*		tophorizon_;
    EM::Horizon*		bottomhorizon_; 
};

}; //namespace


#endif
