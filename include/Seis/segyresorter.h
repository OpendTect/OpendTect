#ifndef segyscanner_h
#define segyscanner_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segyresorter.h,v 1.3 2011-03-25 15:02:34 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "seisposkey.h"
#include "multiid.h"
namespace PosInfo { class CubeDataPos; }


namespace SEGY
{
class DirectReader;

/*!\brief Re-sorts SEG-Y files, input must be 'scanned'.
 
  For 2D, use 'Crl' when you want trace number

 */

mClass ReSorter : public Executor
{
public:

    mClass Setup
    {
    public:

			Setup(Seis::GeomType gt,const MultiID&,
			      const char* outputfnm);

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(MultiID,inpkey)
	mDefSetupMemb(BufferString,outfnm)
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
    BufferString	msg_;
    od_int64		nrdone_;
    od_int64		totnr_;

    PosInfo::CubeDataPos& cdp_;
    int			filefirstidx_;

    int			wrapUp();
    bool		getCurPos(BinID&);
    bool		toNext();
    bool		createOutput(const BinID&);
    bool		openOutputFile(int);

};

} // namespace

#endif
