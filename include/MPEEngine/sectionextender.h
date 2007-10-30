#ifndef sectionextender_h
#define sectionextender_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectionextender.h,v 1.16 2007-10-30 16:53:35 cvskris Exp $
________________________________________________________________________

-*/

#include "task.h"
#include "emposid.h"
#include "sets.h"
#include "cubesampling.h"

class BinID;
class BinIDValue;

namespace EM
{
    class EMObject;
};

namespace MPE
{

class SectionSourceSelector;

class SectionExtender : public SequentialTask
{
public:
    				SectionExtender(const EM::SectionID& si = -1);
    EM::SectionID		sectionID() const;

    virtual void		reset();
    virtual void		setDirection( const BinIDValue& );
    virtual const BinIDValue*	getDirection() const;

    void			setStartPositions(const TypeSet<EM::SubID> ns);
    void			excludePositions(const TypeSet<EM::SubID>*); 
    bool			isExcludedPos(const EM::SubID&) const; 
    int				nextStep();
    
    void			extendInVolume(const BinID& bidstep,
    					       float zstep);

    const TypeSet<EM::SubID>&	getAddedPositions() const;
    const TypeSet<EM::SubID>&	getAddedPositionsSource() const;

    virtual const CubeSampling& getExtBoundary() const;
    void			setExtBoundary(const CubeSampling&);
    void			unsetExtBoundary();

    virtual int			maxNrPosInExtArea() const { return -1; }
    virtual void		preallocExtArea() {}

    const char*			errMsg() const;
    virtual void		fillPar(IOPar&) const {}
    virtual bool		usePar(const IOPar&) { return true; }

protected:
    void			addTarget(const EM::SubID& target,
	    				  const EM::SubID& src );

    TypeSet<EM::SubID>		addedpos_;
    TypeSet<EM::SubID>		addedpossrc_;
    TypeSet<EM::SubID>		startpos_;
    
    const TypeSet<EM::SubID>*	excludedpos_;

    CubeSampling		extboundary_;

    const EM::SectionID		sid_;
    BufferString		errmsg;
};

};

#endif

