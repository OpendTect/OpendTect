#ifndef seispreload_h
#define seispreload_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2009
 RCS:           $Id$
________________________________________________________________________


-*/

#include "seismod.h"
#include "bufstringset.h"
#include "multiid.h"
#include "ranges.h"
#include "task.h"
class IOObj;

namespace Seis
{

mClass(Seis) PreLoader
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
    			//!< Line 2D only.

    bool		loadVol() const;
    bool		loadLines() const;
    bool		loadLines(const BufferStringSet& lnms,
	    			  const BufferStringSet& attrnms) const;
    bool		loadPS3D(const Interval<int>* inlrg=0) const;
    bool		loadPS2D(const char* lnm=0) const;	//!< null => all
    bool		loadPS2D(const BufferStringSet&) const;

    void		unLoad() const;
    const char*		errMsg() const			{ return errmsg_.str();}

    static void		load(const IOPar&,TaskRunner* tr=0);
    			//!< Seis.N.[loadObj_fmt]
    static void		loadObj(const IOPar&,TaskRunner* tr=0);
    			//!< sKey::ID() and optional subselections
    void		fillPar(IOPar&) const;

    static const char*	sKeyLines();
    static const char*	sKeyAttrs();

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

