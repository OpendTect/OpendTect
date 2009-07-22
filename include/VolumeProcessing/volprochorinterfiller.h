#ifndef volprochorinterfiller_h
#define volprochorinterfiller_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id: volprochorinterfiller.h,v 1.5 2009-07-22 16:01:19 cvsbert Exp $
________________________________________________________________________

-*/

#include "multiid.h"
#include "samplingdata.h"
#include "volprocchain.h"

class BinID;
namespace EM { class EMObject; class Horizon; }

namespace VolProc
{
/*! Fills a volume with values. The top and bottom of the volume are either
    the survey top/bottom or horizons. The values are fixed at the top
    boundary (either horizon or survey top) and change either with a fixed
    gradient or to a fixed value at the bottom boundary. */

    
mClass HorInterFiller : public Step
{
public:
    static void			initClass();
    
    				~HorInterFiller();
				HorInterFiller(Chain&);

    bool			isOK() const;

    const char*			type() const { return sKeyType(); }
    bool			needsInput(const HorSampling& ) const
                                	{ return true; }			

    bool 			setTopHorizon(const MultiID*);
    const MultiID*		getTopHorizonID() const;
    
    bool			setBottomHorizon(const MultiID*);	
    const MultiID*		getBottomHorizonID() const;

    float			getTopValue() const;
    void			setTopValue(float);

    bool			usesGradient() const;
    void			useGradient(bool);
    				//!<If false, bottom value will be used

    float			getBottomValue() const;
    void			setBottomValue(float);
    				
    float			getGradient() const;
    void			setGradient(float);
    
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    
    static const char*		sKeyType() { return "HorInterFiller"; }
    static const char*		sUserName() { return "Horizon based painter"; }
    
protected:
    bool			prefersBinIDWise() const        { return true; }
    bool			computeBinID(const BinID&, int);
    bool                        prepareComp(int)		{ return true; }

    static const char*		sKeyTopHorID()	{ return "Top horizon"; }
    static const char*		sKeyBotHorID()	{ return "Bottom horizon"; }
    static const char*		sKeyTopValue()	{ return "Top Value"; }
    static const char*		sKeyBotValue()	{ return "Bottom Value"; }
    static const char*		sKeyGradient()  { return "Gradient"; }
    static const char*		sKeyUseGradient() { return "Use Gradient"; }

  
    EM::Horizon*		loadHorizon(const MultiID&) const;
    static Step*      		create(Chain&);

    float    			topvalue_;    
    float			bottomvalue_;
    EM::Horizon*		tophorizon_;
    EM::Horizon*		bottomhorizon_; 
    bool			usegradient_;
    float			gradient_;
};

}; //namespace


#endif
