#ifndef sectionadjuster_h
#define sectionadjuster_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: sectionadjuster.h,v 1.6 2005-03-17 14:55:55 cvsnanne Exp $
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
				    : sectionid_(sid)
				    , stopbelowthrhold_(false)
				    , thresholdval_(0.5)	{}

    EM::SectionID		sectionID() const 	{ return sectionid_; }

    virtual void		reset() 		{}
    virtual void		setExtender(const SectionExtender*) {}

    int				nextStep()		{ return 0; }
    const char*			errMsg() const  
    				{ return errmsg_[0] ? errmsg_ : 0; }

    const PositionScoreComputer* getComputer(int idx) const;
    PositionScoreComputer*	getComputer(int idx);
    int				nrComputers() const;

    void			setThresholdValue(float val)
				{ thresholdval_ = val; }
    float			getThresholdValue() const
    				{ return thresholdval_; }
    void			doStopBelowThreshold(bool yn)
				{ stopbelowthrhold_ = yn; }
    bool			stopBelowThreshold() const
				{ return stopbelowthrhold_; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
    BufferString		errmsg_;
    EM::SectionID		sectionid_;
    float			thresholdval_;
    bool			stopbelowthrhold_;

    ObjectSet<PositionScoreComputer> computers_;

    static const char*		adjusterstr;
    static const char*		thresholdstr;
    static const char*		stoptrackstr;
};

}; // namespace MPE

#endif
