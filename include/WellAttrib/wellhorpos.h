#ifndef wellhorpos_h
#define wellhorpos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jul 2010
 RCS:           $Id: wellhorpos.h,v 1.4 2010-08-23 09:57:59 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "emposid.h"

/*!brief used to give well info at horizon intersection. !*/

namespace Well { class Track; }
class BinID;
class BinIDValueSet;
class MultiID;

mClass WellHorPos
{
public:
    				WellHorPos(const Well::Track&);

    void			intersectWellHor(BinIDValueSet&) const;
    				//get BinIDs at Well/Horizon intersection
    				//in principle, only one position per well but 
    				//there may be more than one intersection 
    				//( faults )
    void			setHorizon(const EM::ObjectID& emid)
				{ horid_ = emid; }

protected:

    const Well::Track&		track_;
    TypeSet<BinID>		wellbids_;
    EM::ObjectID 	 	horid_;

    void			intersectBinIDsHor(BinIDValueSet&) const;
    void			intersectBinIDHor(const BinID&,float&) const;
    void 			transformWellCoordsToBinIDs();
};

#endif
