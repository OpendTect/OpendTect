#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "maddefs.h"
#include "iopar.h"
#include "typeset.h"
#include "seisinfo.h"
#include "commondefs.h"

class od_stream;

namespace ODMad
{

mExpClass(Madagascar) RSFHeader : public IOPar
{
public:

    			RSFHeader(){};
			~RSFHeader(){};

    enum Format		{ NativeFloat, NativeInt, AsciiFloat, AsciiInt, Other };
    			mDeclareEnumUtils(Format);

    bool		read(const char* fnm);
    bool		read(od_istream&);
    bool		write(const char* fnm) const;
    bool		write(od_ostream&) const;

    int			nrDims() const;

    int			nrVals(int dim) const;
    SamplingData<int>	getSampling(int dim) const;
    SamplingData<float>	getFSampling(int dim) const;

    void		setNrVals(int dim,int nr);
    void		setSampling(int dim, const SamplingData<int>&);
    void		setFSampling(int dim, const SamplingData<float>&);

    const char*		getDataSource() const;
    void		setDataSource(const char*);

    Format		getDataFormat() const;
    void		setDataFormat(Format);

    const char*		getTrcHeaderFile() const;
    void		setTrcHeaderFile(const char*);

    const char*		getODVersion() const;
    void		setODVersion(const char*);

private:

};


mExpClass(Madagascar) TrcHdrDef : public IOPar
{
public:
    			TrcHdrDef();

    int			size_;
    bool		isbinary_;

    static const char*	sKeySize;
    static const char*	sKeyTrcNr;
    static const char*	sKeyOffset;
    static const char*	sKeyScalco;
    static const char*	sKeyDelRt;
    static const char*	sKeyNs;
    static const char*	sKeyDt;
    static const char*	sKeyXcdp;
    static const char*	sKeyYcdp;
    static const char*	sKeyInline;
    static const char*	sKeyCrossline;
    static const char*	sKeySP;
    static const char*	sKeySPScale;


    static int  StdSize();    
    static int  StdIdxTrcNr();
    static int  StdIdxOffset();
    static int  StdIdxScalco();
    static int  StdIdxDelRt();
    static int  StdIdxNs();
    static int  StdIdxDt();
    static int  StdIdxXcdp();
    static int  StdIdxYcdp();
    static int  StdIdxInline();
    static int  StdIdxCrossline();
    static int  StdIdxSP();
    static int  StdIdxSPScale();
};


/*!\brief describes one trace header */
mExpClass(Madagascar) TrcHeader : public TypeSet<int>
{
public:
			TrcHeader(bool is2d,const TrcHdrDef& def);


    bool		fillTrcInfo(SeisTrcInfo&) const;
    bool		useTrcInfo(const SeisTrcInfo&);

    bool		read(od_istream&);
    void		write(od_ostream&) const;

protected:

    bool		is2d_;
    const TrcHdrDef&    trchdrdef_;

    template <class T>
    void		getFld(int idx, T val) const
    			{ if ( (*this)[idx] ) val = (T)(*this)[idx]; }
};


mExpClass(Madagascar) TrcHdrStrm
{
public:
			TrcHdrStrm(bool is2d,bool read,const char* fnm,
					TrcHdrDef& def);

    bool		initRead();
    bool		initWrite() const;

    TrcHeader*          readNextTrc();
    bool		writeNextTrc(const TrcHeader&) const;

protected:

    bool		is2d_;
    od_stream&		strm_;
    TrcHdrDef&		trchdrdef_;
    RSFHeader*		rsfheader_;
    BufferString	errmsg_;
};


} // namespace
