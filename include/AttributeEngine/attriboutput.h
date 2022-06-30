#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		07-10-1999
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "refcount.h"

#include "bufstringset.h"
#include "datapointset.h"
#include "ranges.h"
#include "seistype.h"
#include "trckeyzsampling.h"
#include "uistring.h"

class BinDataDesc;
class BinIDValueSet;
class RegularSeisDataPack;
class SeisTrc;
class SeisTrcInfo;
class SeisTrcBuf;
class SeisTrcWriter;
class Scaler;
namespace Seis { class SelData; }

namespace Attrib
{
class DataHolder;
class Data2DHolder;
class Processor;

/*!
\brief Base class for attribute Output.
*/

mExpClass(AttributeEngine) Output : public ReferencedObject
{
public:
				Output();

    virtual bool		getDesiredVolume(TrcKeyZSampling&) const
				{ return true; }
    virtual bool		useCoords() const		{ return false;}
    virtual bool		wantsOutput(const BinID&) const; // overrule it
    virtual bool		wantsOutput(const Coord&) const; // or this one

    virtual const RegularSeisDataPack*	getDataPack() const	{ return 0; }
    virtual RegularSeisDataPack*	getDataPack(float)	{ return 0; }

    virtual void		getDesiredOutputs( TypeSet<int>& outputs ) const
				{ outputs = desoutputs_; }
    void			setDesiredOutputs( const TypeSet<int>& outputs )
				{ desoutputs_ = outputs; }

    virtual TypeSet<Interval<int> > getLocalZRanges(const BinID&,float,
						    TypeSet<float>&) const;
    virtual TypeSet<Interval<int> > getLocalZRanges(const Coord&,float,
						    TypeSet<float>&) const;
    virtual void		collectData(const DataHolder&,float step,
					    const SeisTrcInfo&)		 = 0;
    virtual SeisTrc*		getTrc()		{ return 0; }
    virtual bool		writeTrc()		{ return true; }
    virtual void		deleteTrc()		{}
    const Seis::SelData&	getSelData()		{ return *seldata_; }
    Pos::GeomID			curGeomID() const;
    virtual void		adjustInlCrlStep(const TrcKeyZSampling&) {}
    virtual bool		finishWrite()		{ return false; }

    static const char*		outputstr();
    static const char*		cubekey();
    static const char*		tskey();
    static const char*		surfkey();
    static const char*		scalekey();
    static const char*		varzlinekey();

    virtual const char*		errMsg() const	{ return ""; }

protected:
    virtual			~Output();

    Seis::SelData*		seldata_;
    TypeSet<int>		desoutputs_;
    void			doSetGeometry(const TrcKeyZSampling&);
    void			ensureSelType(Seis::SelType);
};


#define mImplDefAttribOutputFns(typ) \
    bool			wantsOutput( const typ& t ) const override \
				{ return Output::wantsOutput(t); } \
    TypeSet<Interval<int> >	getLocalZRanges( const typ& t,float f, \
					TypeSet<float>& ts ) const override \
				{ return Output::getLocalZRanges(t,f,ts); }

/*!
\brief Attribute DataPack Output.
*/

mExpClass(AttributeEngine) DataPackOutput : public Output
{
public:
				DataPackOutput(const TrcKeyZSampling&);

    const RegularSeisDataPack*	getDataPack() const override;
    RegularSeisDataPack*	getDataPack(float) override;

    bool			getDesiredVolume(
					    TrcKeyZSampling&) const override;
    void			setGeometry( const TrcKeyZSampling& cs )
				{ doSetGeometry(cs); }
    void			setUndefValue( float v )	{ udfval_ = v; }

    bool			wantsOutput(const BinID&) const override;
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
						TypeSet<float>&) const override;
				mImplDefAttribOutputFns(Coord)
    void			collectData(const DataHolder&,float step,
					    const SeisTrcInfo&) override;
    void			adjustInlCrlStep(
					    const TrcKeyZSampling&) override;

protected:
    TrcKeyZSampling		desiredvolume_;
    TrcKeyZSampling		dcsampling_;	//can differ from desiredvolume_
				//(special cases with decimated cubes smaller
				//than desired display)
    TypeSet< Interval<int> >	sampleinterval_;
    RegularSeisDataPack*	output_;
    float			udfval_;

    void			init(float refstep,const BinDataDesc* bdd=0);
};


/*!
\brief Seismic trace storage Output.
*/

mExpClass(AttributeEngine) SeisTrcStorOutput : public Output
{ mODTextTranslationClass(Attrib::SeisTrcStorOutput)
public:
				SeisTrcStorOutput(const TrcKeyZSampling&,
						  const Pos::GeomID);
				~SeisTrcStorOutput();

    virtual bool		doInit();
    virtual void		set2D( bool yn = true )		{ is2d_ = yn; }
    bool			useCoords() const override	{ return false;}
    bool			getDesiredVolume(
					TrcKeyZSampling&) const override;
    bool			wantsOutput(const BinID&) const override;
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
					     TypeSet<float>&) const override;
				mImplDefAttribOutputFns(Coord)
    bool			setStorageID(const MultiID&);
    void			setGeometry( const TrcKeyZSampling& cs )
				{ doSetGeometry(cs); }

    bool			doUsePar(const IOPar&);
    bool			finishWrite() override;
    void			collectData(const DataHolder&,float step,
					    const SeisTrcInfo&) override;
    bool			writeTrc() override;
    SeisTrc*			getTrc() override	{ return trc_; }
    void			setTrcGrow( bool yn )	{ growtrctosi_ = yn; }
    void			setOutpTypes(const TypeSet<Seis::DataType>& typ)
				{ outptypes_ = typ; }
    const BufferStringSet&	getOutpNames() const	{ return outpnames_; }
    void			setOutpNames( const BufferStringSet& nms )
				{ outpnames_ = nms; }

    const char*			errMsg() const override
				{ return errmsg_.getFullString(); }

    static const char*		seisidkey();
    static const char*		attribkey();
    static const char*		inlrangekey();
    static const char*		crlrangekey();
    static const char*		depthrangekey();

protected:

    MultiID&			storid_;
    TrcKeyZSampling		desiredvolume_;
    TypeSet< Interval<int> >	sampleinterval_;
    IOPar*			auxpars_;
    bool			is2d_;
    uiString			errmsg_;

    SeisTrc*			trc_;
    SeisTrcWriter*		writer_;
    BinID			prevpos_;
    bool			storinited_;
    BufferString		attribname_;
    Scaler*			scaler_;
    TypeSet<Seis::DataType>	outptypes_;
    bool			growtrctosi_;
    BufferString		datatype_;
    BufferStringSet		outpnames_;

    float			writez0shift_;
    friend class		Processor;

public:
    void			deleteTrc() override;
				//only if you do not use writeTrc

};


/*!
\brief 2D trace Output with variable Z range.
*/

mExpClass(AttributeEngine) Trc2DVarZStorOutput : public SeisTrcStorOutput
{ mODTextTranslationClass(Attrib::Trc2DVarZStorOutput)
public:
				Trc2DVarZStorOutput(Pos::GeomID,
						    DataPointSet*,float);

    bool			doInit() override;
    void			set2D(bool) override		{}
    bool			useCoords() const override	{ return true; }

    bool			wantsOutput(const Coord&) const override;
    TypeSet< Interval<int> >	getLocalZRanges(const Coord&,float,
						TypeSet<float>&) const override;
				mImplDefAttribOutputFns(BinID)
    void			setTrcsBounds(Interval<float>);

    bool			finishWrite() override;
    void			collectData(const DataHolder&,float step,
					    const SeisTrcInfo&) override;
    void			setMaxDistBetwTrcs( float maxdist )
						{ maxdisttrcs_ = maxdist; }
protected:

    const TrcKeyZSampling		getCS();

    DataPointSet*		poszvalues_;
    float			stdtrcsz_;
    float			stdstarttime_;
    float			outval_;
    float			maxdisttrcs_;
};


/*!
\brief Simple 2D Output
*/

mExpClass(AttributeEngine) TwoDOutput : public Output
{
public:
				TwoDOutput(const Interval<int>&,
					   const Interval<float>&,
					   Pos::GeomID);
				~TwoDOutput();

    bool			doInit();
    bool			wantsOutput(const BinID&) const override;
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
						TypeSet<float>&) const override;
				mImplDefAttribOutputFns(Coord)
    void			setGeometry(const Interval<int>&,
					    const Interval<float>&);
    bool			getDesiredVolume(
					    TrcKeyZSampling&) const override;
    void			setOutput(Data2DHolder&);

    void			collectData(const DataHolder&,float step,
					    const SeisTrcInfo&) override;
    const char*			errMsg() const override
				{ return errmsg_.str(); }

protected:

    TypeSet< Interval<int> >	sampleinterval_;
    BufferString		errmsg_;

    Data2DHolder*		output_;
};


/*!
\brief Output at discrete locations ( For example a pickset. )
*/

mExpClass(AttributeEngine) LocationOutput : public Output
{
public:
				LocationOutput(BinIDValueSet&);
				~LocationOutput() {};

    bool			getDesiredVolume(
					TrcKeyZSampling&) const override
				{ return true;}

    bool			wantsOutput(const BinID&) const override;
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
						TypeSet<float>&) const override;
				mImplDefAttribOutputFns(Coord)
    void			collectData(const DataHolder&,float step,
					    const SeisTrcInfo&) override;
    void			setPossibleBinIDDuplic() { arebiddupl_ = true; }

    //TODO : check where we want to put this: output?
    static const char*		filenamekey();
    static const char*		locationkey();
    static const char*		attribkey();
    static const char*		surfidkey();

protected:
    BinIDValueSet&		bidvalset_;

    bool			arebiddupl_;

    void			computeAndSetVals(const DataHolder&,
						  float,float*);
    bool			areBIDDuplicated() const;
};


/*!
\brief Output at an assorted selection of traces.
*/

mExpClass(AttributeEngine) TrcSelectionOutput : public Output
{
public:
				TrcSelectionOutput(const BinIDValueSet&,
						   float outval =0);
				~TrcSelectionOutput() ;

    bool			getDesiredVolume(
					TrcKeyZSampling&) const override;
    bool			wantsOutput(const BinID&) const override;
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
						TypeSet<float>&) const override;
				mImplDefAttribOutputFns(Coord)
    void			setOutput(SeisTrcBuf*);
    void			setTrcsBounds(Interval<float>);
    void			collectData(const DataHolder&,float,
					    const SeisTrcInfo&) override;
    void			setGeomID(Pos::GeomID);

protected:
    const BinIDValueSet&	bidvalset_;
    SeisTrcBuf*			outpbuf_;
    float			outval_;
    float			stdtrcsz_;
    float			stdstarttime_;
};


/*!
\brief Output at a selection of locations.
*/

mExpClass(AttributeEngine) TableOutput : public Output
{
public:
				TableOutput(DataPointSet&,int);
				~TableOutput();

    bool			getDesiredVolume(
					TrcKeyZSampling&) const override;

    bool			useCoords() const override;
    bool			wantsOutput(const BinID&) const override;
    bool			wantsOutput(const Coord&) const override;
    bool			wantsOutput(const TrcKey&) const;

    void			collectData(const DataHolder&,float step,
					    const SeisTrcInfo&) override;

    //ABI frozen: special function for 6.0 only, replaces the virtual one above
    void			collectDataSpecial60(const DataHolder&,
						     float step,
						     const SeisTrcInfo&,
						     const TrcKey&);

    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
						TypeSet<float>&) const override;
    TypeSet< Interval<int> >	getLocalZRanges(const Coord&,float,
						TypeSet<float>&) const override;
    TypeSet< Interval<int> >	getLocalZRanges(const TrcKey&,float,
						TypeSet<float>&) const;

    void			setMaxDistBetwTrcs( float maxdist )
						{ maxdisttrcs_ = maxdist; }
    void			setMedianDistBetwTrcs(float mediandist);
    void			setPossibleBinIDDuplic() { arebiddupl_ = true; }
    void			initPairsTable();

protected:
    DataPointSet&		datapointset_;

    bool			arebiddupl_;
    int				firstattrcol_;
    float			maxdisttrcs_;
    float			mediandisttrcs_;
    TypeSet<float>		distpicktrc_;

    void			computeAndSetVals(const DataHolder&,float,
						  float,float*);
    bool			areBIDDuplicated() const;
    void			addLocalInterval(TypeSet<Interval<int> >&,
						 TypeSet<float>&,
						 int,float) const;

    struct PosAndRowIDPair
    {
	Pos::GeomID		gid_;
	Pos::TraceID		tid_;
	DataPointSet::RowID	rid_;

				PosAndRowIDPair()
				    : gid_(-1)
				    , tid_(-1)
				    , rid_(-1)			{};

				PosAndRowIDPair( Pos::GeomID gid,
						 Pos::TraceID tid,
						 DataPointSet::RowID rid )
				    : gid_(gid)
				    , tid_(tid)
				    , rid_(rid)			{};

	bool			operator == ( PosAndRowIDPair other ) const
				{ return other.gid_ == gid_ &&
					 other.tid_ == tid_ &&
					 other.rid_ == rid_;
				}

	bool			operator > ( PosAndRowIDPair other ) const
				{ return gid_ > other.gid_
				    || (gid_==other.gid_ && tid_>other.tid_)
				    || (gid_==other.gid_ && tid_==other.tid_
							 && rid_>other.rid_); }

	bool			matchesTrcKey( const TrcKey& tkey ) const
				{ return gid_ == tkey.geomID()
				      && tid_ == tkey.trcNr(); }
    };

    TypeSet<PosAndRowIDPair>	parpset_;
};


} // namespace Attrib


