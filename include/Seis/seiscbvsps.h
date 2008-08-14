#ifndef seiscbvsps_h
#define seiscbvsps_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seiscbvsps.h,v 1.14 2008-08-14 14:08:05 cvsbert Exp $
________________________________________________________________________

-*/

#include "seispsread.h"
#include "seispswrite.h"
class IOPar;
class BinID;
class Coord;
class SeisTrcTranslator;
class CBVSSeisTrcTranslator;


/*!\brief Implementation class: I/O from a CBVS pre-stack seismic data store.

  Every (in)line is a CBVS cube. A gather corresponds to one crossline/trace
  number). 
  Because CBSV seismics is inline-sorted, the crossline number is stored
  as inline in the cube. Upon retrieval actual BinID and Coord are restored.

  In 2D, things are a bit more 'normal'. Every trace number is an inline
  and the crosslines are simply sequence numbers for the vaious offsets.

 */

class SeisCBVSPSIO
{
public:
    			SeisCBVSPSIO(const char* dirnm);
			// Check errMsg() to see failure
    virtual		~SeisCBVSPSIO();

    BufferString	get2DFileName(const char* lnm) const;

    void		usePar(const IOPar&);
    void		setPrefStorType( DataCharacteristics::UserType ut )
						{ reqdtype_ = ut; }

    bool		getSampleNames(BufferStringSet&) const;
    bool		setSampleNames(const BufferStringSet&) const;

protected:

    mutable BufferString		errmsg_;
    const BufferString			dirnm_;
    const BufferString			selmask_;
    int					nringather_;
    DataCharacteristics::UserType	reqdtype_;
    CBVSSeisTrcTranslator*		tr_;

    void		close();
    bool		dirNmOK(bool forread) const;
    SeisTrc*		readNewTrace(int) const;
    bool		goTo(int,int) const;
    bool		prepGather(int,SeisTrcBuf&) const;
    bool		startWrite(const char*,const SeisTrc&);

    const char*		ext() const	{ return selmask_.buf()+1; }

};


/*!\brief reads from a CBVS pre-stack seismic data store. */

class SeisCBVSPS3DReader : public SeisPS3DReader
		         , public SeisCBVSPSIO
{
public:

    			SeisCBVSPS3DReader(const char* dirnm,int inl=mUdf(int));
			// Check errMsg() to see failure
			~SeisCBVSPS3DReader();

    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    const char*		errMsg() const		{ return errmsg_.buf(); } 

    const PosInfo::CubeData& posData() const	{ return posdata_; }
    bool		getSampleNames( BufferStringSet& bss ) const
			{ return SeisCBVSPSIO::getSampleNames(bss); }

    void		usePar( const IOPar& i ) { SeisCBVSPSIO::usePar(i); }

protected:

    PosInfo::CubeData&	posdata_;

    void		addInl(int);
    bool		mkTr(int) const;
    bool		getGather(int,SeisTrcBuf&) const;
    SeisTrc*		getNextTrace(const BinID&,const Coord&) const;

    mutable int		curinl_;

};


/*!\brief reads from a CBVS pre-stack seismic data store. */

class SeisCBVSPS2DReader : public SeisPS2DReader
		         , public SeisCBVSPSIO
{
public:

    			SeisCBVSPS2DReader(const char* dirnm,const char* lnm);
			// Check errMsg() to see failure
			~SeisCBVSPS2DReader();

    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    const char*		errMsg() const		{ return errmsg_.buf(); } 

    const PosInfo::Line2DData& posData() const	{ return posdata_; }
    bool		getSampleNames( BufferStringSet& bss ) const
			{ return SeisCBVSPSIO::getSampleNames(bss); }

    void		usePar( const IOPar& i ) { SeisCBVSPSIO::usePar(i); }

protected:

    PosInfo::Line2DData& posdata_;

};


/*!\brief writes to a CBVS 3D pre-stack seismic data store.

 Note: Can make new data stores and append new inlines to existing.
 Will replace any existing inlines.

 */

class SeisCBVSPS3DWriter : public SeisPSWriter
		         , public SeisCBVSPSIO
{
public:

    			SeisCBVSPS3DWriter(const char* dirnm);
			// Check errMsg() to see failure
			~SeisCBVSPS3DWriter();

    bool		put(const SeisTrc&);
    const char*		errMsg() const		{ return errmsg_.buf(); } 
    void		close();

    bool		setSampleNames( const BufferStringSet& bss ) const
			{ return SeisCBVSPSIO::setSampleNames(bss); }

    void		usePar( const IOPar& i ) { SeisCBVSPSIO::usePar(i); }

protected:

    BinID&		prevbid_;

    bool		newInl(const SeisTrc&);

};


/*!\brief writes to a CBVS 2D pre-stack seismic data store.

 Note: Can make new data stores, add new lines and replace existing.

 */

class SeisCBVSPS2DWriter : public SeisPSWriter
		         , public SeisCBVSPSIO
{
public:

    			SeisCBVSPS2DWriter(const char* dirnm,const char* lnm);
			// Check errMsg() to see failure

    bool		put(const SeisTrc&);
    const char*		errMsg() const		{ return errmsg_.buf(); } 
    void		close();

    bool		setSampleNames( const BufferStringSet& bss ) const
			{ return SeisCBVSPSIO::setSampleNames(bss); }

    void		usePar( const IOPar& i ) { SeisCBVSPSIO::usePar(i); }

protected:

    int			prevnr_;
    BufferString	lnm_;
    bool		ensureTr(const SeisTrc&);

};


#endif
