#ifndef madprocflow_h
#define madprocflow_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Dec 2007
 * ID       : $Id: madprocflow.h,v 1.2 2007-12-20 16:18:54 cvsbert Exp $
-*/

#include "iopar.h"
#include "namedobj.h"
#include "bufstringset.h"


namespace ODMad
{

class ProcFlow : public ::NamedObject
{
public:

    enum IOType		{ Vol, VolPS, Line, LinePS, Madagascar, None };

    			ProcFlow(const char* nm=0);
    			~ProcFlow();

    IOPar&		input()			{ return inpiop_; }
    const IOPar&	input() const		{ return inpiop_; }
    const IOPar&	output() const		{ return outiop_; }
    IOPar&		output() 		{ return outiop_; }
    const BufferStringSet& procs() const	{ return procs_; }
    BufferStringSet&	procs()			{ return procs_; }

    IOType		ioType( bool inp ) const
    			{ return ioType( inp ? inpiop_ : outiop_ ); }
    void		setIOType( bool inp, IOType iot )
    			{ setIOType( inp ? inpiop_ : outiop_, iot ); }
    int			size() const		{ return procs_.size(); }
    const char*		proc( int idx ) const	{ return procs_.get(idx); }
    void		addProc( const char* s ) { procs_.add( s ); }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static IOType	ioType(const IOPar&);
    static void		setIOType(IOPar&,IOType);

protected:

    IOPar		inpiop_;
    IOPar		outiop_;
    BufferStringSet	procs_;

};

} // namespace ODMad

#endif
