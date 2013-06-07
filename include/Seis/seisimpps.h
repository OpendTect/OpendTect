#ifndef seisimpps_h
#define seisimpps_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jan 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "strmdata.h"
#include "bufstringset.h"
#include "multiid.h"
#include "executor.h"
class SeisTrc;
class SeisTrcWriter;
class SeisPSImpLineBuf;


/*!\brief Manages import of acquisition-sorted PS data.
          May take loads of memory

  If the import is handling more data than what fits in memory (and this is
  easily reached), you should set the maximum possible inline offset. This
  allows writing and disposing of inlines of gathers that can never be reached
  afterwards.

 */


mClass SeisPSImpDataMgr
{
public:

    			SeisPSImpDataMgr(const MultiID& pswrid);
    virtual		~SeisPSImpDataMgr();

    void		add(SeisTrc*);		//!< trc becomes mine
    void		endReached();		//!< call after last 'add'
    bool		needWrite() const	{ return !towrite_.isEmpty(); }
    bool		writeGather();
    			//!< Write possibly incomplete gather if !needWrite()
    bool		isEmpty() const		{ return lines_.isEmpty(); }

    int			maxInlOffset() const	{ return maxinloffs_; }
    void		setMaxInlOffset( int i ) { maxinloffs_ = i; }
    void		setSampleNames( const BufferStringSet& bss )
						{ samplenms_ = bss; }

    const char*		errMsg() const		{ return errmsg_.str(); }
    const SeisTrcWriter* trcWriter() const	{ return wrr_; }
    bool		constGatherSize() const	{ return gathersize_ > 0; }

protected:

    ObjectSet<SeisPSImpLineBuf>	lines_;
    MultiID			wrid_;
    SeisTrcWriter*		wrr_;
    int				maxinloffs_;
    TypeSet<int>		towrite_;
    BufferStringSet		samplenms_;

    int				gathersize_;
    mutable BufferString	errmsg_;

    void			updateStatus(int);
};


#endif
