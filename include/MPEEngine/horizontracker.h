#ifndef horizontracker_h
#define horizontracker_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "task.h"
#include "thread.h"
#include "emposid.h"
#include "refcount.h"
#include "position.h"

namespace EM {
    class Fault;
    class Horizon;
    class Horizon2D;
    class Horizon3D;
}

template <class T> class Array2DImpl;

class InlCrlSystem;
class SeisTrc;



namespace MPE {

/*!
\brief SequentialTask to autotrack EM::Horizon.
*/

mExpClass HorizonAutoTracker : public SequentialTask
{
public:	
    				HorizonAutoTracker( EM::Horizon& );
                                ~HorizonAutoTracker();
    
    void			setInlCrlSystem( const InlCrlSystem& );
    
    bool			init();
    
    const ObjectSet<EM::Fault>&	getFaults() const 	{ return faults_; }
    void			addFault(EM::Fault&);
    void			removeFault(EM::Fault&);
    
    int				doStep();
    
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    
    const char*			errMsg() const { return errmsg_.str(); }
    const EM::Horizon&		horizon() const { return horizon_; }
    
    static FixedString		sNrHorizons()	{ return "Nr Horizons"; }
    static FixedString		sKeyHorizonID()	{ return "Horizon ID"; }
    
    struct			TrackTarget
    {
	friend			class HorizonAutoTrackerTask;
	TrackTarget( HorizonAutoTracker* );
	void			reset();
	
	static unsigned char	getSourceMask(EM::SubID from,EM::SubID to);
	static unsigned char	cSourcePP() { return 1; }
	static unsigned char	cSourcePS() { return 2; }
	static unsigned char	cSourcePN() { return 4; }
	static unsigned char	cSourceSP() { return 8; }
	static unsigned char	cSourceSN() { return 16; }
	static unsigned char	cSourceNP() { return 32; }
	static unsigned char	cSourceNS() { return 64; }
	static unsigned char	cSourceNN() { return 128; }
	
        void			computeProposal();
        
        EM::PosID		pid_;
        
        float			score_;
        float			proposedz_;
        unsigned char		sources_;
        
        HorizonAutoTracker*	tracker_;
	
	bool			istarget_;
	bool			needsrecalc_;
	bool			isvalid_;
    };

    
protected:
    void			removeAll();
    
    RowCol			getArrayPos(const EM::PosID&) const;
    
        
    float			getInterpolZ(const EM::PosID&) const;
    void			computeScores();
    void			addSeed(EM::SectionID,EM::SubID);
    int				addPossibleTarget(EM::SectionID,EM::SubID);
				//Returns index (to array) if target should be computed
    void			getSources(const EM::PosID&,
					   TypeSet<EM::PosID>&) const;
    
    const SeisTrc*		getTrace(EM::SubID) const;
    				//Waits until it is loaded
    void			requestTrace(EM::SubID);
    				//Tell that you want it to be loaded
    
    BufferString		errmsg_;
    
    EM::Horizon2D*		hor2d_;
    EM::Horizon3D*		hor3d_;
    EM::Horizon&		horizon_;
    
    ObjectSet<EM::Fault>	faults_;
    
    /* For 2D, there is one row in the arrays per line in the survey, and one
       column per trace, using the step of the horizon.
       For 3D, there is one row per inline in the survey, and one column per
       trace, both with the step of the horizon. */
    
    PtrMan<Array2DImpl<TrackTarget> >		targetarray_;
    
    PtrMan<Array2DImpl<unsigned char> >		history_;
    
    PtrMan<Array2DImpl<PtrMan<SeisTrc> > >	trcs_;
    PtrMan<Array2DImpl<PtrMan<SeisTrc> > >	diptrcs_;
    PtrMan<Array2DImpl<bool> >			tracewanted_;
    
    RowCol					arrayorigin_;
    RowCol					step_;
    
    RefMan<InlCrlSystem>			inlcrlsystem_;
};
    

}; // namespace MPE

#endif

