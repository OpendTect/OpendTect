#ifndef seisimpbpsif_h
#define seisimpbpsif_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jan 2008
 RCS:		$Id: seisimpbpsif.h,v 1.11 2012-08-03 13:00:36 cvskris Exp $
________________________________________________________________________

-*/

#include "seismod.h"
#include "strmdata.h"
#include "bufstringset.h"
#include "multiid.h"
#include "executor.h"
class SeisTrc;
class SeisPSImpDataMgr;


/*!\brief reads a BPSIF pre-stack data exchange file into a PS data store

 The BPSIF format is defined as follows:
 * First line is "#BPSIF#"
 * Then, a header follows with only empty lines and lines starting with '#'.
   Some are mandatory (see below).
 * The header stops at a line starting with "#----"
 * After the header, lines follow with one shot per line. The line starts with
   data specific for the shot, then the data per receiver (group) follows.
 * Mandatory data for each shot is number, X and Y. For receivers: X and Y.

  The header needs to define the 'attributes' with '#Value.' lines. Example:

#Value.1.0: SHOT NUMBER
#Value.1.1: SOURCE X
#value.1.2: SOURCE Y
#Value.1.3: SOURCE ATTRIBUTE-1 = SOURCE PRESSURE (PSI)
#Value.1.4: SOURCE ATTRIBUTE-2 = SPEED THROUGH WATER (KNOTS)

#Value.2.1: STREAMER-1 NEAR RECEIVER X
#Value.2.2: STREAMER-1 NEAR RECEIVER Y
#Value.2.3: RECEIVER ATTRIBUTE-1 = STREAMER-1 NEAR RECEIVER DEPTH (METRES)
#Value.2.4: RECEIVER ATTRIBUTE-2 = STREAMER-1 NEAR RECEIVER AMBIENT RMS NOISE

Typically, lines will be extremely long. The number of values per line is:
3 + nr_opt_shot_attribs + N * (2 + nr_opt_rcv_attribs).

This class will read a BPSIF and store the data in a pre-stack data store.
Notes:
* The data will be sorted if the data store requires that
* The input filename may contain glob expression, e.g. line*.dat. These files
  will be handled in alphabetical order. This can be important if the data needs
  to be sorted.
* The 'real' name of the attribute is the part after the '=' sign, but it's not
  mandatory. If there is no '=' sign, the whole string after the ':' is used.

  The actual stoarge is handled by the SeisPSImpDataMgr object - see more
  comments there.

*/

mClass(Seis) SeisImpBPSIF : public Executor
{
public:

    			SeisImpBPSIF(const char* filenm,const MultiID&);
    virtual		~SeisImpBPSIF();
    void		setMaxInlOffset(int);

    const char*		message() const;
    od_int64		nrDone() const		{ return nrshots_; }
    const char*		nrDoneText() const	{ return "Shots handled"; }
    int			nextStep();

    int			nrFiles() const		{ return fnames_.size(); }
    			// Available after first nextStep():
    const BufferStringSet& header() const	{ return hdrlines_; }
    const BufferStringSet& shotAttrs() const	{ return shotattrs_; }
    const BufferStringSet& rcvAttrs() const	{ return rcvattrs_; }
    			// Available after execution:
    bool		isIrregular() const	{ return irregular_; }
    int			nrShots() const		{ return nrshots_; }
    int			nrRcvrs() const		{ return nrrcvpershot_; }
    int			nrRejected() const	{ return nrrejected_; }

protected:

    int			curfileidx_;
    int			nrshots_;
    int			nrrejected_;
    int			nrrcvpershot_;
    bool		binary_;
    bool		irregular_;
    bool		endofinput_;
    StreamData		cursd_;
    BufferStringSet	fnames_;
    BufferStringSet	hdrlines_;
    BufferStringSet	shotattrs_;
    BufferStringSet	rcvattrs_;
    SeisPSImpDataMgr&	datamgr_;
    mutable BufferString errmsg_;

    bool		open(const char*);
    bool		openNext();
    bool		readFileHeader();
    void		addAttr(BufferStringSet&,char*);
    int			readAscii();
    int			readBinary();
    int			addTrcsAscii(const SeisTrc&,char*);
    bool		addTrcsBinary(const SeisTrc&);
    int			fileEnded();
    int			writeData();

};


#endif

