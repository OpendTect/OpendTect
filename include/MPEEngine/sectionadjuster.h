#ifndef sectionadjuster_h
#define sectionadjuster_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: sectionadjuster.h,v 1.5 2005-03-15 16:53:29 cvsnanne Exp $
________________________________________________________________________

-*/

#include "basictask.h"
#include "emposid.h"

class IOPar;
namespace EM { class EMObject; };

namespace MPE
{

class PositionScoreComputer;
class SectionExtender;

class SectionAdjuster : public BasicTask
{
public:
				SectionAdjuster(EM::EMObject&,
						const EM::SectionID& sid=-1)
				    : sectionid(sid)
				    , stopbelowthrhold(false)
				    , thresholdval(0.5)	{}

    EM::SectionID		sectionID() const 	{ return sectionid; }

    virtual void		reset() 		{}
    virtual void		setExtender(const SectionExtender*) {}

    int				nextStep()		{ return 0; }
    const char*			errMsg() const  
    				{ return errmsg[0] ? errmsg : 0; }

    const PositionScoreComputer* getComputer(int idx) const;
    PositionScoreComputer*	getComputer(int idx);
    int				nrComputers() const;

    void			setThresholdValue(float val)
				{ thresholdval = val; }
    float			getThresholdValue() const
    				{ return thresholdval; }
    void			doStopBelowThreshold(bool yn)
				{ stopbelowthrhold = yn; }
    bool			stopBelowThreshold() const
				{ return stopbelowthrhold; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
    BufferString		errmsg;
    EM::SectionID		sectionid;
    float			thresholdval;
    bool			stopbelowthrhold;

    ObjectSet<PositionScoreComputer>	computers;
};

}; // namespace MPE

#endif
