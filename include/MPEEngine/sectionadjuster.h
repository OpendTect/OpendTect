#ifndef sectionadjuster_h
#define sectionadjuster_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectionadjuster.h,v 1.2 2005-01-18 13:14:20 kristofer Exp $
________________________________________________________________________

-*/

#include "basictask.h"

#include "emposid.h"

class BinIDValue;


namespace MPE
{

class SectionExtender;

class SectionAdjuster : public BasicTask
{
public:
    			SectionAdjuster( EM::EMObject&,
					 const EM::SectionID& sid = -1)
			    : sectionid( sid )				{}

    EM::SectionID	sectionID() const { return sectionid; }

    virtual void	reset() {}
    virtual void	setExtender( const SectionExtender* ) {}

    int			nextStep() { return 0; }
    const char*		errMsg() const  { return errmsg[0] ? errmsg : 0; }

protected:
    BufferString		errmsg;
    EM::SectionID		sectionid;
};

};

#endif

