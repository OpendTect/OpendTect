#ifndef seiscopy_h
#define seiscopy_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2014
 RCS:		$Id$
________________________________________________________________________

-*/


#include "seismod.h"
#include "executor.h"

class IOObj;
class SeisTrcReader;
class SeisTrcWriter;
class Scaler;
namespace Seis { class RangeSelData; }


/*!\brief Copies cubes. TODO */

mExpClass(Seis) SeisCubeCopier : public Executor
{
public:


			SeisCubeCopier(const IOObj& inobj,const IOObj& outobj,
					const IOPar&);

			~SeisCubeCopier();

    od_int64		totalNr() const		{ return totalnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const;
    int			nextStep();

protected:

    const IOObj&		inioobj_;
    const IOObj&		outioobj_;
    SeisTrcReader*		rdr_;
    SeisTrcWriter*		wrr_;
    uiString			msg_;

    od_int64			nrdone_;
    od_int64			totalnr_;

};


/*!\brief Copies line sets.

  Expects an IOPar with subpars Line.N.*, each of those containing:
  - sKey::GeomID()
  - sKey::TrcRange()
  - sKey::ZRange()
  Failing to provide one of those will exclude the line.

*/

mExpClass(Seis) SeisLineSetCopier : public Executor
{
public:


			SeisLineSetCopier(const IOObj& inobj,const IOObj& outob,
					  const IOPar&);

			~SeisLineSetCopier();

    od_int64		totalNr() const		{ return totalnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const;
    int			nextStep();

protected:

    const IOObj&		inioobj_;
    const IOObj&		outioobj_;
    SeisTrcReader*		rdr_;
    SeisTrcWriter*		wrr_;
    uiString			msg_;
    Seis::RangeSelData&		seldata_;

    TypeSet<Pos::GeomID>	selgeomids_;
    TypeSet<StepInterval<int> > trcrgs_;
    TypeSet<StepInterval<float> > zrgs_;
    Scaler*			scaler_;
    int				lineidx_;
    od_int64			nrdone_;
    od_int64			totalnr_;

    bool			initNextLine();

};



#endif
