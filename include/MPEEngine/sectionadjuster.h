#ifndef sectionadjuster_h
#define sectionadjuster_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          January 2005
 RCS:           $Id: sectionadjuster.h,v 1.7 2005-07-21 20:58:12 cvskris Exp $
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
				SectionAdjuster( const EM::SectionID& sid=-1);
    EM::SectionID		sectionID() const;

    virtual void		reset();

    void			setPositions(const TypeSet<EM::SubID>& );
    void			setDirection(const BinIDValue*);

    int				nextStep();
    const char*			errMsg() const;

    const PositionScoreComputer* getComputer(int idx) const;
    PositionScoreComputer*	getComputer(int idx);
    int				nrComputers() const;

    void			setThresholdValue(float val);
    float			getThresholdValue() const;
    void			doStopBelowThreshold(bool yn);
    bool			stopBelowThreshold() const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
    const BinIDValue*		direction;
    TypeSet<EM::SubID>		pids;
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
