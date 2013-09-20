#ifndef seisreaderset_h
#define seisreaderset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "seisread.h"
#include "manobjectset.h"


/*!\brief advances and reads from multipe SeisTrcReader's

The get functions will be syncronised. Only positions that are common to all
are returned.

*/

mExpClass(Seis) SeisTrcReaderSet : public ManagedObjectSet<SeisTrcReader>
{
public:

    bool		is2D() const; //!< determined by first reader

    bool		prepareWork(Seis::ReadMode rm=Seis::Prod);

    int			get(ObjectSet<SeisTrcInfo>&);
    				//!< not optional: has to be called explicitly
    bool		get(ObjectSet<SeisTrc>&);

    void		setComponent(int);

    const char*		errMsg() const		{ return errmsg_; }

protected:

    mutable BufferString errmsg_;

    bool		getSingle(int,SeisTrcInfo&,int& res);
    BinID		getBinID(int,const SeisTrcInfo&) const;

};


#endif

