#ifndef seiscbvsps_h
#define seiscbvsps_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seiscbvsps.h,v 1.11 2008-01-21 17:56:13 cvsbert Exp $
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

    bool		dirNmOK(bool forread) const;
    bool		goTo(SeisTrcTranslator*,const BinID&,int) const;
    bool		prepGather(SeisTrcTranslator*,int,SeisTrcBuf&) const;

    BufferString	get2DFileName(const char* lnm) const;

    bool		getSampleNames(BufferStringSet&) const;
    bool		setSampleNames(const BufferStringSet&) const;

protected:


    mutable BufferString errmsg_;
    const BufferString	dirnm_;
    const BufferString	selmask_;
    const char*		ext() const	{ return selmask_.buf()+1; }

};


/*!\brief reads from a CBVS pre-stack seismic data store. */

class SeisCBVSPS3DReader : public SeisPS3DReader
		         , private SeisCBVSPSIO
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

protected:

    PosInfo::CubeData&	posdata_;

    void		addInl(int);
    bool		mkTr(int) const;
    bool		getGather(int,SeisTrcBuf&) const;
    SeisTrc*		getNextTrace(const BinID&,const Coord&) const;

    mutable CBVSSeisTrcTranslator* curtr_;
    mutable int		curinl_;

};


/*!\brief reads from a CBVS pre-stack seismic data store. */

class SeisCBVSPS2DReader : public SeisPS2DReader
		         , private SeisCBVSPSIO
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

protected:

    PosInfo::Line2DData&	posdata_;

    CBVSSeisTrcTranslator*	tr_;

};


/*!\brief writes to a CBVS 3D pre-stack seismic data store.

 Note: Can make new data stores and append new inlines to existing.
 Will replace any existing inlines.

 */

class SeisCBVSPS3DWriter : public SeisPSWriter
		         , private SeisCBVSPSIO
{
public:

    			SeisCBVSPS3DWriter(const char* dirnm);
			// Check errMsg() to see failure
			~SeisCBVSPS3DWriter();

    void		setPrefStorType( DataCharacteristics::UserType ut )
						{ reqdtype_ = ut; }
    void		usePar(const IOPar&);
    void		close();

    bool		put(const SeisTrc&);
    const char*		errMsg() const		{ return errmsg_.buf(); } 

    bool		setSampleNames( BufferStringSet& bss ) const
			{ return SeisCBVSPSIO::setSampleNames(bss); }

protected:

    BinID&				prevbid_;
    int					nringather_;
    DataCharacteristics::UserType	reqdtype_;
    DataCharacteristics			dc_;
    SeisTrcTranslator*			tr_;

    bool				newInl(const SeisTrc&);

};


/*!\brief writes to a CBVS 2D pre-stack seismic data store.

 Note: Can make new data stores, add new lines and replace existing.

 */

class SeisCBVSPS2DWriter : public SeisPSWriter
		         , private SeisCBVSPSIO
{
public:

    			SeisCBVSPS2DWriter(const char* dirnm);
			// Check errMsg() to see failure
			~SeisCBVSPS2DWriter();

    void		setPrefStorType( DataCharacteristics::UserType ut )
						{ reqdtype_ = ut; }
    void		usePar(const IOPar&);
    void		close();

    bool		put(const SeisTrc&);
    const char*		errMsg() const		{ return errmsg_.buf(); } 

    bool		setSampleNames( BufferStringSet& bss ) const
			{ return SeisCBVSPSIO::setSampleNames(bss); }

protected:

    int				nringather_;
    DataCharacteristics::UserType	reqdtype_;
    DataCharacteristics		dc_;
    SeisTrcTranslator*		tr_;

};


#endif
