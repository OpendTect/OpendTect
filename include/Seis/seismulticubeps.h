#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "seispsread.h"
#include "seispsioprov.h"
#include "uistring.h"
class SeisTrcReader;


/*!\brief PS data store reader based on multiple 3D CBVS cubes */

mExpClass(Seis) MultiCubeSeisPSReader : public SeisPS3DReader
{ mODTextTranslationClass(MultiCubeSeisPSReader);
public:

			MultiCubeSeisPSReader(const char* fnm);
			// Check errMsg() to see failure
			~MultiCubeSeisPSReader();

    SeisTrc*		getTrace(const BinID&,int) const override;
    bool		getGather(const BinID&,SeisTrcBuf&) const override;
    uiString		errMsg() const override		{ return errmsg_; }

    const PosInfo::CubeData& posData() const override	{ return posdata_; }
    bool		getSampleNames(BufferStringSet&) const override
			{ return false; }

    void		usePar(const IOPar&) override;

    void		addReader( SeisTrcReader* rdr, float offs )
			{ rdrs_ += rdr; offs_ += offs; }

    bool		getFrom(const char* fnm);
    bool		putTo(const char* fnm) const;

    static bool		readData(const char* fnm,ObjectSet<MultiID>&,
				  TypeSet<float>&,TypeSet<int>&,
				  uiString& emsg);
    static bool		writeData(const char* fnm,const ObjectSet<MultiID>&,
				  const TypeSet<float>&,const TypeSet<int>&,
				  uiString& emsg);

protected:

    PosInfo::CubeData&		posdata_;
    ObjectSet<SeisTrcReader>	rdrs_;
    TypeSet<float>		offs_;
    TypeSet<int>		comps_;
    mutable uiString		errmsg_;

    void			getCubeData(const SeisTrcReader&,
					    PosInfo::CubeData&) const;

};


mExpClass(Seis) MultiCubeSeisPS3DTranslator : public SeisPS3DTranslator
{			       isTranslator(MultiCube,SeisPS3D)
public:
			mDefEmptyTranslatorConstructor(MultiCube,SeisPS3D)
			~MultiCubeSeisPS3DTranslator();

    virtual bool	isUserSelectable( bool fr ) const override
			{ return fr; }
    const char*		defExtension() const override	{ return "mcps"; }
};
