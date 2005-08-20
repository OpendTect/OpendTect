#ifndef sectionextender_h
#define sectionextender_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectionextender.h,v 1.8 2005-08-20 19:01:00 cvskris Exp $
________________________________________________________________________

-*/

#include "basictask.h"
#include "emposid.h"
#include "sets.h"
#include "trackstattbl.h"

namespace EM
{
    class EMObject;
};

namespace MPE
{

class SectionSourceSelector;

class SectionExtender : public BasicTask
{
public:
    				SectionExtender( const EM::SectionID& si = -1 );
    EM::SectionID		sectionID() const;

    virtual void		reset();
    virtual void		setDirection( const BinIDValue& );
    virtual const BinIDValue*	getDirection() const;

    void			setStartPositions(const TypeSet<EM::SubID> ns);
    void			setTrackStatTbl(const TrackingStatusTable* t)
    				    { trkstattbl = t; }

    int				nextStep();
    
    void			extendInVolume(const BinID& bidstep,
    					       float zstep);

    const TypeSet<EM::SubID>&	getAddedPositions() const;
    const TypeSet<EM::SubID>&	getAddedPositionsSource() const;

    const char*			errMsg() const;

protected:
    void			addTarget( const EM::SubID& target,
	    				   const EM::SubID& src );

    TypeSet<EM::SubID>		addedpos;
    TypeSet<EM::SubID>		addedpossrc;
    TypeSet<EM::SubID>		startpos;
    const TrackingStatusTable*	trkstattbl;

    const EM::SectionID		sid;
    BufferString		errmsg;
};

};

#endif

