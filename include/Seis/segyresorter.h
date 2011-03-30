#ifndef segyscanner_h
#define segyscanner_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segyresorter.h,v 1.4 2011-03-30 11:47:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "seisposkey.h"
#include "multiid.h"
#include "strmdata.h"
#include "bufstringset.h"
namespace PosInfo { class CubeData; class CubeDataPos; }


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
	mDefSetupMemb(int,newfileeach)
	mDefSetupMemb(bool,inlnames)

	Interval<int>	getInlRg(int,const PosInfo::CubeData&) const;
	BufferString	getFileName(const Interval<int>&) const;

	mutable int	curfnr_;

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
    StreamData		sdout_;
    Interval<int>	curinlrg_;
    bool		needwritefilehdrs_;

    ObjectSet<StreamData> inpsds_;
    TypeSet<int>	fidxs_;
    BufferStringSet	inpfnms_;

    unsigned char	hdrbuf_[3600];
    unsigned char*	trcbuf_;
    od_int64		trcbytes_;

    PosInfo::CubeDataPos& cdp_;

    int			wrapUp();
    bool		getCurPos(BinID&);
    bool		toNext();
    bool		createOutput(const BinID&);
    bool		openOutputFile();
    int			ensureFileOpen(int);
    bool		readData(int,int);
    bool		writeData();

};

} // namespace

#endif
