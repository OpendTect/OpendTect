#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			SeisTrcReaderSet();
			~SeisTrcReaderSet();

    bool		is2D() const; //!< determined by first reader

    bool		prepareWork(Seis::ReadMode rm=Seis::Prod);

    int			getTrcInfos(ObjectSet<SeisTrcInfo>&);
				//!< not optional: has to be called explicitly
    bool		getTrcs(ObjectSet<SeisTrc>&);

    void		setComponent(int);

    uiString		errMsg() const		{ return errmsg_; }

protected:

    mutable uiString	errmsg_;

    bool		getSingle(int,SeisTrcInfo&,int& res);
    BinID		getBinID(int,const SeisTrcInfo&) const;

};
