#ifndef segyscanner_h
#define segyscanner_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "executor.h"
#include "seisposkey.h"
#include "multiid.h"
#include "strmdata.h"
#include "bufstringset.h"
namespace Pos		{ class Filter; }
namespace PosInfo	{ class CubeData; class CubeDataPos; }


namespace SEGY
{
class DirectDef;
class FileDataSet;
class DirectReader;

/*!\brief Re-sorts SEG-Y files, input must be 'scanned'.
 
  For 2D, use 'Crl' when you want trace number

 */

mExpClass(Seis) ReSorter : public Executor
{
public:

    mExpClass(Seis) Setup
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

    void		setFilter(const Pos::Filter&);

    const char*		message() const		{ return msg_; }
    const char*		nrDoneText() const	{ return "Traces handled"; }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totnr_; }
    int			nextStep();

    const DirectDef&	dDef() const;
    const FileDataSet&	fds() const;

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

    TypeSet<BinID>	binids_;
    PosInfo::CubeDataPos& cdp_;
    Pos::Filter*	posfilt_;

    int			fillBinIDs();
    int			wrapUp();
    bool		getCurPos(BinID&);
    bool		toNext();
    bool		getNext(const BinID&,int&,int&) const;
    bool		createOutput(const BinID&);
    bool		openOutputFile();
    int			ensureFileOpen(int);
    bool		readData(int,int);
    bool		writeData();

};

} // namespace

#endif

