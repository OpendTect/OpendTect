#ifndef seisimpps_h
#define seisimpps_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Jan 2008
 RCS:		$Id: seisimpps.h,v 1.1 2008-01-14 12:06:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "strmdata.h"
#include "bufstringset.h"
#include "multiid.h"
#include "executor.h"
class SeisTrc;
class SeisPSWriter;
class SeisPSImpLineBuf;


/*!\brief Manages import of acquisition-sorted PS data.
          May take loads of memory

  If the import is handling more data than what fits in memory (and this is
  easily reached), you should set the maximum possible inline offset. This
  allows writing and disposing of inlines of gathers that can never be reached
  afterwards.

 */


class SeisPSImpDataMgr
{
public:

    			SeisPSImpDataMgr(const MultiID& pswrid);
    virtual		~SeisPSImpDataMgr();

    void		add(SeisTrc*);		//!< trc becomes mine
    void		endReached();		//!< call after last 'add'
    bool		needWrite() const	{ return writeupto_ >= 0; }
    bool		writeGather();
    			//!< Write possibly incomplete gather if !needWrite()
    bool		isEmpty() const		{ return lines_.isEmpty(); }

    int			maxInlOffset() const	{ return maxinloffs_; }
    void		setMaxInlOffset( int i ) { maxinloffs_ = i; }
    void		setSampleNames( const BufferStringSet& bss )
						{ samplenms_ = bss; }

    const char*		errMsg() const		{ return errmsg_.buf(); }
    const SeisPSWriter*	psWriter() const	{ return pswrr_; }
    bool		constGatherSize() const	{ return gathersize_ > 0; }

protected:

    ObjectSet<SeisPSImpLineBuf>	lines_;
    MultiID			wrid_;
    SeisPSWriter*		pswrr_;
    int				maxinloffs_;
    int				writeupto_;
    BufferStringSet		samplenms_;

    int				gathersize_;
    mutable BufferString	errmsg_;

    void			updateStatus(int);
};


#endif
