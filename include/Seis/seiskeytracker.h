#ifndef seiskeytracker_h
#define seiskeytracker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2016
 RCS:           $Id$
________________________________________________________________________


-*/

#include "seiscommon.h"
#include "binid.h"
#include "od_iosfwd.h"


namespace Seis
{

/*!\brief builds a record of visited positions; stores only changes that
	    cannot be predicted from previous changes. */

mExpClass(Seis) KeyTracker
{
public:

			KeyTracker(od_ostream&,GeomType,bool bin=false);
    virtual		~KeyTracker()	    { finish(); }
    void		reset(); //!< does not reposition stream

    void		add(int trcnr,float offs=0.f);
    void		add(const BinID&,float offs=0.f);

    od_int64		nrDone() const	    { return nrhandled_; };
    void		finish(); //!< will be done on destruction anyway

protected:

    od_ostream&		strm_;
    const bool		is2d_;
    const bool		isps_;
    const bool		binary_;

    od_int64		nrhandled_;
    BinID		prevbid_;
    bool		diriscrl_;
    Index_Delta_Type	step_;
    bool		finished_;
    int			offsidx_;
    TypeSet<float>	offsets_;
    bool		offsetschanged_;

    void		addFirst(const BinID&,float);
    void		addFirstFollowUp(const BinID&,float);
    void		addNext(const BinID&,float);
    void		addNextPS(const BinID&,float);
    void		recordOffsets();
    void		recordPos(const BinID&,bool);
    void		checkCurOffset(float);
    void		getNewIncs(const BinID&);
    void		getNextPredBinID(BinID&) const;
    bool		isSamePos(const BinID&,const BinID&) const;

};


} // namespace Seis

#endif
