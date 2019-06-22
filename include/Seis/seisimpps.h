#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2008
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "bufstringset.h"
#include "dbkey.h"
#include "executor.h"
#include "uistring.h"
class SeisTrc;
class SeisPSImpLineBuf;
namespace Seis { class Storer; }


/*!\brief Manages import of acquisition-sorted PS data.
          May take loads of memory

  If the import is handling more data than what fits in memory (and this is
  easily reached), you should set the maximum possible inline offset. This
  allows writing and disposing of inlines of gathers that can never be reached
  afterwards.

 */


mExpClass(Seis) SeisPSImpDataMgr
{ mODTextTranslationClass(SeisPSImpDataMgr);
public:

    mUseType( Seis,	Storer );

			SeisPSImpDataMgr(const DBKey&);
    virtual		~SeisPSImpDataMgr();

    void		add(SeisTrc*);		//!< trc becomes mine
    void		endReached();		//!< call after last 'add'
    bool		needWrite() const	{ return !towrite_.isEmpty(); }
    uiRetVal		writeGather();
			//!< Write possibly incomplete gather if !needWrite()
    bool		isEmpty() const		{ return lines_.isEmpty(); }

    int			maxInlOffset() const	{ return maxinloffs_; }
    void		setMaxInlOffset( int i ) { maxinloffs_ = i; }
    void		setSampleNames( const BufferStringSet& bss )
						{ samplenms_ = bss; }

    uiString		errMsg() const		{ return errmsg_; }
    const Storer*	storer() const		{ return storer_; }
    bool		constGatherSize() const	{ return gathersize_ > 0; }

protected:

    ObjectSet<SeisPSImpLineBuf>	lines_;
    DBKey			wrid_;
    Storer*			storer_;
    int				maxinloffs_;
    TypeSet<int>		towrite_;
    BufferStringSet		samplenms_;

    int				gathersize_;
    mutable uiString		errmsg_;

    void			updateStatus(int);

};
