#ifndef madhdr_h
#define madhdr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jul 2012
 RCS:           $Id$
________________________________________________________________________

-*/

#include "iopar.h"
#include "typeset.h"
#include "seisinfo.h"
#include "commondefs.h"


namespace ODMad
{

mClass(Madagascar) RSFHeader : public IOPar
{
public:

    			RSFHeader();
			~RSFHeader();

    enum Format		{ NativeFloat, NativeInt, AsciiFloat, AsciiInt, Other };

    bool		read(const char* fnm);
    bool		read(std::istream&);
    bool		write(const char* fnm) const;
    bool		write(std::ostream&) const;

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


mClass(Madagascar) TrcHdrDef : public IOPar
{
public:

    int			size_;
    bool		isbinary_;

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
mClass(Madagascar) TrcHeader : public TypeSet<int>
{
public:
			TrcHeader(bool is2d,const TrcHdrDef& def);


    bool		fillTrcInfo(SeisTrcInfo&) const;
    bool		useTrcInfo(const SeisTrcInfo&);

    bool		read(const char* fnm);
    void		write(char* fnm) const;

protected:

    bool		is2d_;
    const TrcHdrDef&    trchdrdef_;
};


mClass(Madagascar) TrcHeaderSet
{
public:
			TrcHeaderSet(bool is2d,TrcHdrDef& def,
					const RSFHeader* rsfheader);

    int			nrSamples() const;

    TrcHeader*		read(const char* fnm);
    bool		write(const TrcHeader&,char* fnm) const;

protected:

    bool		is2d_;
    TrcHdrDef&		trchdrdef_;
    const RSFHeader*	rsfheader_;
};


} // namespace

#endif
