/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: jobdescprov.cc,v 1.1 2004-10-25 07:26:20 bert Exp $
________________________________________________________________________

-*/

#include "mmwpprov.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "cubesampling.h"
#include <iostream>


JobDescProv::JobDescProv( const IOPar& iop )
	: inpiopar_(*new IOPar(iop))
{
}


JobDescProv::~JobDescProv()
{
    delete &inpiopar_;
}


KeyReplaceJobDescProv::KeyReplaceJobDescProv( const IOPar& iop, const char* key,
					      const BufferStringSet& nms )
	: JobDescProv(iop)
	, key_(key)
    	, names_(nms)
{
}


void KeyReplaceJobDescProv::getJob( int jid, IOPar& iop ) const
{
    iop = inpiopar_;
    iop.set( key_, objName(jid) );
}


const char* KeyReplaceJobDescProv::objName( int jid ) const
{
    objnm_ = names_.get( jid );
    return objnm_.buf();
}


void KeyReplaceJobDescProv::dump( std::ostream& strm ) const
{
    strm << "\nKey-replace JobDescProv dump.\n"
	    "The following jobs description keys are available:\n";
    for ( int idx=0; idx<names_.size(); idx++ )
	strm << names_.get(idx) << ' ';
    strm << std::endl;
}


#define mSetInlRgDef() \
    Interval<int> dum; SI().sampling(false).hrg.get( inlrg_, dum )


InlineSplitJobDescProv::InlineSplitJobDescProv( const IOPar& iop,
					     	const char* sk )
    	: JobDescProv(iop)
    	, singlekey_(sk)
    	, inls_(0)
{
    mSetInlRgDef();
    getRange( inlrg_ );
}


InlineSplitJobDescProv::InlineSplitJobDescProv( const IOPar& iop,
			const TypeSet<int>& in, const char* sk )
    	: JobDescProv(iop)
    	, singlekey_(sk)
    	, inls_(new TypeSet<int>(in))
{
    mSetInlRgDef();
}


InlineSplitJobDescProv::~InlineSplitJobDescProv()
{
    delete inls_;
}


void InlineSplitJobDescProv::getRange( StepInterval<int>& rg ) const
{
    if ( *(const char*)singlekey_ )
	inpiopar_.get( singlekey_, rg.start, rg.stop, rg.step );
    else
    {
	inpiopar_.get( sKey::FirstInl, rg.start );
	inpiopar_.get( sKey::LastInl, rg.stop );
	inpiopar_.get( sKey::StepInl, rg.step );
    }

    if ( rg.step < 0 ) rg.step = -rg.step;
    if ( !rg.step ) rg.step = SI().inlStep();
    rg.sort();
}


int InlineSplitJobDescProv::inlNr( int jid ) const
{
    return inls_ ? (*inls_)[jid] : inlrg_.atIndex(jid);
}


void InlineSplitJobDescProv::getJob( int jid, IOPar& iop ) const
{
    iop = inpiopar_;
    const int inl = inlNr( jid );
    if ( *(const char*)singlekey_ )
	iop.set( singlekey_, inl, inl, inlrg_.step );
    else
    {
	iop.set( sKey::FirstInl, inl );
	iop.set( sKey::LastInl, inl );
    }
}


const char* InlineSplitJobDescProv::objName( int jid ) const
{
    objnm_ = "inline "; objnm_ += inlNr( jid );
    return objnm_.buf();
}


void InlineSplitJobDescProv::dump( std::ostream& strm ) const
{
    strm << "\nInline-split JobDescProv dump.\n";
    if ( !inls_ )
	strm << "Inline range: " << inlrg_.start << '-' << inlrg_.stop
		<< " / " << inlrg_.step;
    else
    {
	strm << "The following inlines are requested:\n";
	for ( int idx=0; idx<inls_->size(); idx++ )
	    strm << (*inls_)[idx] << ' ';
    }
    strm << std::endl;
}
