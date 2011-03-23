#ifndef segyscanner_h
#define segyscanner_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segyresorter.h,v 1.2 2011-03-23 12:00:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "seisposkey.h"
#include "multiid.h"
class SeisTrc;
namespace PosInfo { class LineData; }


namespace SEGY
{
class DirectReader;

/*!\brief Re-sorts SEG-Y files, input must be 'scanned'.
 
  For 2D, use 'Crl' when you want trace number

 */

mClass ReSorter : public Executor
{
public:

    enum Sorting	{ Inl, Crl, Offs };

    mClass Setup
    {
    public:

			Setup(Seis::GeomType gt,const MultiID&,
			      const char* outputfnm);

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(MultiID,inpkey)
	mDefSetupMemb(BufferString,outfnm)
	mDefSetupMemb(Sorting,sortkey1)
	mDefSetupMemb(Sorting,sortkey2)
	mDefSetupMemb(int,nridxsperfile)

    };

    			ReSorter(const Setup&,const char* linename=0);
    			~ReSorter();

    const char*		message() const		{ return msg_; }
    const char*		nrDoneText() const	{ return "Traces handled"; }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totnr_; }
    int			nextStep();

protected:

    const Setup		setup_;

    SEGY::DirectReader*	drdr_;
    SeisTrc&		trc_;
    BufferString	msg_;
    od_int64		nrdone_;
    od_int64		totnr_;

    int			inlidx_;
    int			segidx_;
    int			crlidx_;

    int			wrapUp();
    bool		getCurPos(BinID&);
    bool		createOutput(const BinID&);

};

} // namespace

#endif
