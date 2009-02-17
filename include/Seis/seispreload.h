#ifndef seispreload_h
#define seispreload_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2009
 RCS:           $Id: seispreload.h,v 1.4 2009-02-17 15:12:03 cvsbert Exp $
________________________________________________________________________


-*/

#include "bufstringset.h"
#include "multiid.h"
#include "ranges.h"
#include "task.h"
class IOObj;

namespace Seis
{

mClass PreLoader
{
public:

    			PreLoader( const MultiID& ky, TaskRunner* tr=0 )
			    : id_(ky), tr_(tr)		{}
    void		setID( const MultiID& ky )	{ id_ = ky; }
    const MultiID&	id() const			{ return id_; }
    void		setRunner( TaskRunner& t )	{ tr_ = &t; }

    IOObj*		getIOObj() const;
    Interval<int>	inlRange() const;
    			//!< PS 3D only. If nothing there: ret.start==mUdf(int)
    void		getLineKeys(BufferStringSet&) const;

    bool		loadVol() const;
    bool		loadLines(const BufferStringSet& lnms,
	    			  const BufferStringSet& attrnms) const;
    bool		loadPS3D(const Interval<int>* inlrg=0) const;
    bool		loadPS2D(const char* lnm) const;

    void		unLoad() const;

    const char*		errMsg() const			{ return errmsg_; }

protected:

    MultiID		id_;
    TaskRunner*		tr_;
    TaskRunner		deftr_;
    mutable BufferString errmsg_;

    TaskRunner&		getTr() const
    			{ return *((TaskRunner*)(tr_ ? tr_ : &deftr_)); }
};


} // namespace

#endif
