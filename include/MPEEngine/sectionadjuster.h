#ifndef sectionadjuster_h
#define sectionadjuster_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: sectionadjuster.h,v 1.12 2005-08-29 09:57:44 cvsduntao Exp $
________________________________________________________________________

-*/

#include "basictask.h"
#include "cubesampling.h"
#include "emposid.h"


class IOPar;
namespace EM { class EMObject; };
namespace Attrib { class SelSpec; }

namespace MPE
{

class PositionScoreComputer;
class SectionExtender;

class SectionAdjuster : public BasicTask
{
public:
				SectionAdjuster( const EM::SectionID& sid=-1);
    EM::SectionID		sectionID() const;

    virtual void		reset() {};

    void			setPositions(const TypeSet<EM::SubID>& targets,
	   				     const TypeSet<EM::SubID>* src=0 );
    void			setReferencePosition( const EM::SubID* pos )
    				    { refpos_ = pos; }

    int				nextStep();
    const char*			errMsg() const;

    virtual CubeSampling	getAttribCube(const Attrib::SelSpec&) const;
    virtual void		getNeededAttribs(
	    			    ObjectSet<const Attrib::SelSpec>&) const;

    const PositionScoreComputer* getComputer(int idx) const;
    PositionScoreComputer*	getComputer(int idx);
    int				nrComputers() const;

    void			setThresholdValue(float val);
    float			getThresholdValue() const;
    void			doStopBelowThreshold(bool yn);
    bool			stopBelowThreshold() const;
    bool			extrapolateOnFail() const;
    void			doExtrapolateOnFail(bool yn);

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
    TypeSet<EM::SubID>		pids_;
    TypeSet<EM::SubID>		pidsrc_;
    BufferString		errmsg_;
    EM::SectionID		sectionid_;
    float			thresholdval_;
    bool			stopbelowthrhold_;
    bool			extrapolateonfail_;

    const EM::SubID*		refpos_;
    
    ObjectSet<PositionScoreComputer> computers_;

    static const char*		adjusterstr;
    static const char*		thresholdstr;
    static const char*		stoptrackstr;
    static const char*		extronfailstr;
};

}; // namespace MPE

#endif
