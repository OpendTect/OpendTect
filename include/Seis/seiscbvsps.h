#ifndef seiscbvsps_h
#define seiscbvsps_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seiscbvsps.h,v 1.20 2012-08-03 13:00:36 cvskris Exp $
________________________________________________________________________

-*/

#include "seismod.h"
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

  You can make an instance of this class, to construct file names.

 */

mClass(Seis) SeisCBVSPSIO
{
public:
    			SeisCBVSPSIO(const char* dirnm);
			// Check errMsg() to see failure
    virtual		~SeisCBVSPSIO();
    const char*		errMsg() const		{ return errmsg_.str(); }

    BufferString	get2DFileName(const char* lnm) const;
    bool		get3DFileNames(BufferStringSet&,
	    				const Interval<int>* inlrg=0) const;
    static int		getInlNr(const char* filenm);

    void		usePar(const IOPar&);
    void		setPrefStorType( DataCharacteristics::UserType ut )
						{ reqdtype_ = ut; }

    bool		getSampleNames(BufferStringSet&) const;
    bool		setSampleNames(const BufferStringSet&) const;

    const char*		ext() const	{ return selmask_.buf()+1; }

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

};


/*!\brief reads from a CBVS pre-stack seismic data store. */

mClass(Seis) SeisCBVSPS3DReader : public SeisPS3DReader
		         , public SeisCBVSPSIO
{
public:

    			SeisCBVSPS3DReader(const char* dirnm,int inl=mUdf(int));
			~SeisCBVSPS3DReader();
    const char*		errMsg() const	{ return SeisCBVSPSIO::errMsg(); } 

    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;

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

mClass(Seis) SeisCBVSPS2DReader : public SeisPS2DReader
		         , public SeisCBVSPSIO
{
public:

    			SeisCBVSPS2DReader(const char* dirnm,const char* lnm);
			~SeisCBVSPS2DReader();
    const char*		errMsg() const	{ return SeisCBVSPSIO::errMsg(); } 

    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;

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

mClass(Seis) SeisCBVSPS3DWriter : public SeisPSWriter
		         , public SeisCBVSPSIO
{
public:

    			SeisCBVSPS3DWriter(const char* dirnm);
			~SeisCBVSPS3DWriter();
    const char*		errMsg() const	{ return SeisCBVSPSIO::errMsg(); } 

    bool		put(const SeisTrc&);
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

mClass(Seis) SeisCBVSPS2DWriter : public SeisPSWriter
		         , public SeisCBVSPSIO
{
public:

    			SeisCBVSPS2DWriter(const char* dirnm,const char* lnm);
    const char*		errMsg() const	{ return SeisCBVSPSIO::errMsg(); } 

    bool		put(const SeisTrc&);
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

