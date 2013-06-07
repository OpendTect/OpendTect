#ifndef seis2dlineio_h
#define seis2dlineio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "seistrctr.h"

class IOPar;
class SeisTrc;
class Executor;
class SeisTrcBuf;
template <class T>	class StepInterval;
namespace PosInfo	{ class Line2DData; }
namespace Seis		{ class SelData; }

/*!\brief interface for object that writes 2D seismic data */

mClass Seis2DLinePutter
{
public:
    virtual		~Seis2DLinePutter()	{}

    virtual bool	put(const SeisTrc&)	= 0;
    //!< Return fase on success, err msg on failure
    virtual bool	close()			= 0;
    //!< Return null on success, err msg on failure
    virtual const char* errMsg() const		= 0;
    //!< Only when put or close returns false
    virtual int	nrWritten() const		= 0;
    
};


/*!\brief Provides read/write to/from 2D seismic lines.
	  Only interesting if you want to add your own 2D data I/O. */

mClass Seis2DLineIOProvider
{
public:

    virtual		~Seis2DLineIOProvider()			{}

    virtual bool	isUsable(const IOPar&) const		{ return true; }

    virtual bool	isEmpty(const IOPar&) const		= 0;
    virtual bool	getGeometry(const IOPar&,
				    PosInfo::Line2DData&) const	= 0;
    virtual Executor*	getFetcher(const IOPar&,SeisTrcBuf&,int,
	    			   const Seis::SelData* sd=0)	= 0;
    virtual Seis2DLinePutter* getReplacer(const IOPar&)	= 0;
    virtual Seis2DLinePutter* getAdder(IOPar&,const IOPar* prev,
	    				const char* lgrpnm)	= 0;

    virtual bool	getTxtInfo(const IOPar&,BufferString&,
	    			   BufferString&) const		{ return false;}
    virtual bool	getRanges(const IOPar&,StepInterval<int>&,
	    			   StepInterval<float>&) const	{ return false;}

    static const char*	sKeyLineNr;

    virtual void	removeImpl(const IOPar&) const		= 0;

    const char*		type() const			{ return type_.buf(); }

protected:

			Seis2DLineIOProvider( const char* t )
    			: type_(t)				{}

    const BufferString	type_;
};


ObjectSet<Seis2DLineIOProvider>& S2DLIOPs();
//!< Sort of factory. Add a new type via this function.


//------
//! Translator mechanism is only used for selection etc.

mClass TwoDSeisTrcTranslator : public SeisTrcTranslator
{			isTranslator(TwoD,SeisTrc) public:
			TwoDSeisTrcTranslator( const char* s1, const char* s2 )
			: SeisTrcTranslator(s1,s2)      {}

    const char*		defExtension() const		{ return "2ds"; }
    bool		implRemove(const IOObj*) const;
    bool		initRead_();		//!< supporting getRanges()
    bool		initWrite_(const SeisTrc&)	{ return false; }
    bool		isReadDefault() const		{ return true; }

    bool		implRename( const IOObj*,const char*,
	    			    const CallBack* cb=0) const;

};


#endif
