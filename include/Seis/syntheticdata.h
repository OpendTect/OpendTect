#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki/Bruno
 Date:		July 2013
________________________________________________________________________

-*/

#include "seismod.h"
#include "flatview.h"
#include "datapack.h"
#include "stratsynthgenparams.h"
#include "reflectivitymodel.h"

class SeisTrc;
class TimeDepthModel;


mStruct(Seis) SynthFVSpecificDispPars
{
    			SynthFVSpecificDispPars()
			: overlap_(1)	{}
    ColTab::MapperSetup	vdmapper_;
    ColTab::MapperSetup	wvamapper_;
    BufferString	ctab_;
    float 		overlap_;
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
};


/*! brief the basic synthetic dataset. contains the data cubes*/
mExpClass(Seis) SyntheticData : public NamedObject
{
public:
    					~SyntheticData();

    void				setName(const char*);

    virtual const SeisTrc*		getTrace(int seqnr) const = 0;

    float				getTime(float dpt,int seqnr) const;
    float				getDepth(float time,int seqnr) const;

    const DataPack&			getPack() const {return *datapack_;}
    DataPack&				getPack()	{return *datapack_;}

    ObjectSet<const TimeDepthModel> 	d2tmodels_;
    ObjectSet<const TimeDepthModel>	zerooffsd2tmodels_;

    DataPack::FullID			datapackid_;

    int					id_;
    virtual bool			isPS() const 		= 0;
    virtual bool			hasOffset() const 	= 0;
    virtual bool			isAngleStack() const;
    virtual bool			isAVOGradient() const { return false; }
    virtual SynthGenParams::SynthType	synthType() const	= 0;

    virtual void			useGenParams(const SynthGenParams&);
    virtual void			fillGenParams(SynthGenParams&) const;
    void				useDispPar(const IOPar&);
    void				fillDispPar(IOPar&) const;
    const char*				waveletName() const { return wvltnm_; }
    void				setWavelet( const char* wvltnm )
					{ wvltnm_ = wvltnm; }
    SynthFVSpecificDispPars&		dispPars() 	{ return disppars_; }
    const SynthFVSpecificDispPars&	dispPars() const
							{ return disppars_; }
    RefMan<ReflectivityModelSet>	getRefModels();
    void		setRefModels(const RefMan<ReflectivityModelSet>&);

protected:
					SyntheticData(const SynthGenParams&,
						      DataPack&);

    BufferString 			wvltnm_;
    IOPar				raypars_;
    SynthFVSpecificDispPars		disppars_;

    void				removePack();

   RefMan<DataPack>			datapack_;
   RefMan<ReflectivityModelSet>		refmodels_;
   RefMan<ReflectivityModelSet>		sampledrefmodels_;
};
