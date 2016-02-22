#ifndef seisdatapackwriter_h
#define seisdatapackwriter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "trckeyzsampling.h"
#include "executor.h"
#include "multiid.h"

namespace PosInfo { class CubeData; }
class RegularSeisDataPack;
class SeisTrcWriter;
class SeisTrc;


mExpClass(Seis) SeisDataPackWriter : public Executor
{ mODTextTranslationClass(SeisDataPackWriter);
public:
			SeisDataPackWriter(const MultiID&,
					   const RegularSeisDataPack&,
					   const TypeSet<int>& cubeindices);
			~SeisDataPackWriter();

    void		setSelection(const TrcKeySampling&,
				     const Interval<int>&);

    od_int64		nrDone() const;
    od_int64		totalNr() const;
    uiString		uiMessage() const;
    uiString		uiNrDoneText() const
			{ return tr("Traces written:"); }
    int			nextStep();

private:

   int				nrdone_;
   int				totalnr_;
   const RegularSeisDataPack&	cube_;
   TrcKeySamplingIterator	iterator_;
   MultiID			mid_;
   const PosInfo::CubeData*	trcssampling_;
   SeisTrcWriter*		writer_;
   SeisTrc*			trc_;
   TypeSet<int>			cubeindices_;

   TrcKeySampling		tks_;
   Interval<int>		zrg_;
};


#endif
