#ifndef madprocflow_h
#define madprocflow_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Dec 2007
 * ID       : $Id: madprocflow.h,v 1.1 2007-12-19 14:02:44 cvsbert Exp $
-*/

#include "iopar.h"
#include "bufstringset.h"


namespace ODMad
{

class ProcFlow
{
public:

    enum IOType		{ Vol, VolPS, Line, LinePS, Madagascar, None };

    			ProcFlow();
    			~ProcFlow();

    const IOPar&	input() const		{ return inpiop_; }
    const IOPar&	output() const		{ return outiop_; }
    const BufferStringSet& procs() const	{ return procs_; }
    BufferStringSet&	procs()			{ return procs_; }

    IOType		ioType(bool inp) const;
    void		setIOType(bool inp,IOType);
    int			size() const		{ return procs_.size(); }
    const char*		proc( int idx ) const	{ return procs_.get(idx); }
    void		addProc( const char* s ) { procs_.add( s ); }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:

    IOPar		inpiop_;
    IOPar		outiop_;
    BufferStringSet	procs_;

};

} // namespace ODMad

#endif
