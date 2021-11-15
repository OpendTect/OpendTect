#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2004
________________________________________________________________________

-*/

#include "seismod.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "uistring.h"

class SeisTrcTranslator;
class CBVSSeisTrcTranslator;


/*!\brief Implementation class: I/O from a CBVS prestack seismic data store.

  Every (in)line is a CBVS cube. A gather corresponds to one crossline/trace
  number).
  Because CBSV seismics is inline-sorted, the crossline number is stored
  as inline in the cube. Upon retrieval actual BinID and Coord are restored.

  In 2D, things are a bit more 'normal'. Every trace number is an inline
  and the crosslines are simply sequence numbers for the vaious offsets.

  You can make an instance of this class, to construct file names.

 */

mExpClass(Seis) SeisCBVSPSIO
{ mODTextTranslationClass(SeisCBVSPSIO);
public:
			SeisCBVSPSIO(const char* dirnm);
			// Check errMsg() to see failure
    virtual		~SeisCBVSPSIO();
    uiString		errMsg() const		{ return errmsg_; }

    BufferString	get2DFileName(Pos::GeomID) const;
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

    mutable uiString			errmsg_;
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


/*!\brief reads from a CBVS prestack seismic data store. */

mExpClass(Seis) SeisCBVSPS3DReader : public SeisPS3DReader
				   , public SeisCBVSPSIO
{ mODTextTranslationClass(SeisCBVSPS3DReader);
public:

			SeisCBVSPS3DReader(const char* dirnm,int inl=mUdf(int));
			~SeisCBVSPS3DReader();
    uiString		errMsg() const	{ return SeisCBVSPSIO::errMsg(); }

    SeisTrc*		getTrace(const BinID&,int) const override;
    bool		getGather(const BinID&,SeisTrcBuf&) const override;

    const PosInfo::CubeData& posData() const	{ return posdata_; }
    StepInterval<float>	getZRange() const;
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


/*!\brief reads from a CBVS prestack seismic data store. */

mExpClass(Seis) SeisCBVSPS2DReader : public SeisPS2DReader
				   , public SeisCBVSPSIO
{ mODTextTranslationClass(SeisCBVSPS2DReader);
public:

			SeisCBVSPS2DReader(const char* dirnm,Pos::GeomID);
			SeisCBVSPS2DReader(const char* dirnm,const char* lnm);
			~SeisCBVSPS2DReader();
    uiString		errMsg() const	{ return SeisCBVSPSIO::errMsg(); }

    SeisTrc*		getTrace(const BinID&,int) const override;
    bool		getGather(const BinID&,SeisTrcBuf&) const override;

    const PosInfo::Line2DData& posData() const	{ return posdata_; }
    bool		getSampleNames( BufferStringSet& bss ) const
			{ return SeisCBVSPSIO::getSampleNames(bss); }

    void		usePar( const IOPar& i ) { SeisCBVSPSIO::usePar(i); }

protected:

    PosInfo::Line2DData& posdata_;

    void		init(Pos::GeomID);
};


/*!\brief writes to a CBVS 3D prestack seismic data store.

 Note: Can make new data stores and append new inlines to existing.
 Will replace any existing inlines.

 */

mExpClass(Seis) SeisCBVSPS3DWriter : public SeisPSWriter
		         , public SeisCBVSPSIO
{ mODTextTranslationClass(SeisCBVSPS3DWriter);
public:

			SeisCBVSPS3DWriter(const char* dirnm);
			~SeisCBVSPS3DWriter();
    uiString		errMsg() const	{ return SeisCBVSPSIO::errMsg(); }

    bool		put(const SeisTrc&);
    void		close();

    bool		setSampleNames( const BufferStringSet& bss ) const
			{ return SeisCBVSPSIO::setSampleNames(bss); }

    void		usePar( const IOPar& i ) { SeisCBVSPSIO::usePar(i); }

protected:

    BinID&		prevbid_;

    bool		newInl(const SeisTrc&);

};


/*!\brief writes to a CBVS 2D prestack seismic data store.

 Note: Can make new data stores, add new lines and replace existing.

 */

mExpClass(Seis) SeisCBVSPS2DWriter : public SeisPSWriter
		         , public SeisCBVSPSIO
{ mODTextTranslationClass(SeisCBVSPS2DWriter);
public:

			SeisCBVSPS2DWriter(const char* dirnm,Pos::GeomID);
			SeisCBVSPS2DWriter(const char* dirnm,const char* lnm);
    uiString		errMsg() const	{ return SeisCBVSPSIO::errMsg(); }

    bool		put(const SeisTrc&);
    void		close();

    bool		setSampleNames( const BufferStringSet& bss ) const
			{ return SeisCBVSPSIO::setSampleNames(bss); }

    void		usePar( const IOPar& i ) { SeisCBVSPSIO::usePar(i); }

protected:

    int			prevnr_;
    BufferString	lnm_;
    Pos::GeomID		geomid_;
    bool		ensureTr(const SeisTrc&);

};


