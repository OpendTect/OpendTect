#ifndef sectionextender_h
#define sectionextender_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectionextender.h,v 1.1 2005-01-06 09:25:55 kristofer Exp $
________________________________________________________________________

-*/

#include "basictask.h"
#include "emposid.h"
#include "sets.h"

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
    			SectionExtender( EM::EMObject&,
					 const EM::SectionID& si = -1 )
			    : sectionid( si ) {}

    virtual void	reset() { addedpos.erase(); }
    virtual void	setDirection( const BinIDValue& ) {}
    virtual void	setSelector( const SectionSourceSelector* ) {}
    int			nextStep() { return 0; }

    const char*		errMsg() const { return errmsg[0] ? errmsg : 0; }

    const TypeSet<EM::SubID>&	getAddedPositions() const { return addedpos; }

protected:
    const EM::SectionID	sectionid;
    TypeSet<EM::SubID>	addedpos;
    BufferString	errmsg;
};

};

#endif

