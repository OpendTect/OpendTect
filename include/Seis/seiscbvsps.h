#ifndef seiscbvsps_h
#define seiscbvsps_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seiscbvsps.h,v 1.10 2008-01-17 12:25:27 cvsbert Exp $
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

  Every inline is a CBVS cube. A gather corresponds to one crossline. 
  Because CBSV seismics is inline-sorted, the crossline number is stored
  as inline in the cube. Upon retrieval actual BinID and Coord are restored.

 */

class SeisCBVSPSIO
{
public:

    			SeisCBVSPSIO(const char* dirnm);
			// Check errMsg() to see failure
    virtual		~SeisCBVSPSIO();

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

    bool		getGather(const BinID&,SeisTrcBuf&) const;
    SeisTrc*		getTrace(const BinID&,int) const;
    const char*		errMsg() const		{ return errmsg_.buf(); } 

    const PosInfo::CubeData& posData() const	{ return posdata_; }
    bool		getSampleNames(BufferStringSet&) const;

protected:

    PosInfo::CubeData&	posdata_;

    void		addInl(int);
    bool		mkTr(int) const;
    bool		getGather(int,SeisTrcBuf&) const;
    SeisTrc*		getNextTrace(const BinID&,const Coord&) const;

    mutable CBVSSeisTrcTranslator* curtr_;
    mutable int		curinl_;

};


/*!\brief writes to a CBVS pre-stack seismic data store.

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

    bool		setSampleNames(const BufferStringSet&) const;

protected:

    BinID&				prevbid_;
    int					nringather_;
    DataCharacteristics::UserType	reqdtype_;
    DataCharacteristics			dc_;
    SeisTrcTranslator*			tr_;

    bool				newInl(const SeisTrc&);

};


#endif
