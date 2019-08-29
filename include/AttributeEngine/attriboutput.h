#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "attributeenginecommon.h"
#include "bufstringset.h"
#include "ranges.h"
#include "refcount.h"
#include "seistype.h"
#include "fullsubsel.h"
#include "binid.h"
#include "uistrings.h"

class BinDataDesc;
class BinnedValueSet;
class DataPointSet;
class RegularSeisDataPack;
class SeisTrc;
class SeisTrcInfo;
class SeisTrcBuf;
class TrcKeyZSampling;
class Scaler;
namespace Seis { class SelData; class Storer; }

namespace Attrib
{
class DataHolder;
class Data2DHolder;
class Processor;

/*!
\brief Base class for attribute Output.
*/

mExpClass(AttributeEngine) Output : public RefCount::Referenced
{
public:

    mUseType( Pos,		GeomID );
    mUseType( Survey,		FullSubSel );

				Output();

    virtual bool		getDesiredSubSel(FullSubSel&) const
				{ return true; }
    virtual bool		useCoords(OD::GeomSystem) const	{ return false;}
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
    virtual void		setPossibleSubSel(const FullSubSel&) {}
    virtual bool		finishWrite()		{ return false; }

    static const char*		outputstr();
    static const char*		cubekey();
    static const char*		tskey();
    static const char*		surfkey();
    static const char*		scalekey();
    static const char*		varzlinekey();

    virtual const uiString	errMsg() const
				{ return uiString::empty(); }

protected:
				~Output();

    Seis::SelData*		seldata_;
    TypeSet<int>		desoutputs_;

    void			doSetGeometry(const FullSubSel&);
    void			ensureSelType(Seis::SelType);
};


#define mImplDefAttribOutputFns(typ) \
    virtual bool		wantsOutput( const typ& t ) const \
				{ return Output::wantsOutput(t); } \
    virtual TypeSet<Interval<int> > getLocalZRanges( const typ& t,float f, \
						    TypeSet<float>& ts ) const \
				{ return Output::getLocalZRanges(t,f,ts); }

/*!\brief Attribute DataPack Output */

mExpClass(AttributeEngine) DataPackOutput : public Output
{
public:
				DataPackOutput(const FullSubSel&);
				DataPackOutput(const TrcKeyZSampling&);
    bool			is2D() const;

    const RegularSeisDataPack*	getDataPack() const;
    virtual RegularSeisDataPack*getDataPack(float);

    bool			getDesiredSubSel(FullSubSel&) const;
    void			setGeometry( const FullSubSel& fss )
				{ doSetGeometry(fss); }
    void			setUndefValue( float v )	{ udfval_ = v; }

    bool			wantsOutput(const BinID&) const;
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
						TypeSet<float>&) const;
				mImplDefAttribOutputFns(Coord)
    virtual void		collectData(const DataHolder&,float step,
					    const SeisTrcInfo&);
    void			setPossibleSubSel(const FullSubSel&);

protected:

    FullSubSel		desiredsubsel_;
    FullSubSel		dcfss_;	//can differ from desiredsubsel_
				//(special cases with decimated cubes smaller
				//than desired display)
    TypeSet< Interval<int> >	sampleinterval_;
    RefMan<RegularSeisDataPack>	output_;
    float			udfval_;

    void			init(float refstep,const BinDataDesc* bdd=0);
};


/*!
\brief Seismic trace storage Output.
*/

mExpClass(AttributeEngine) SeisTrcStorOutput : public Output
{ mODTextTranslationClass(Attrib::SeisTrcStorOutput)
public:

				SeisTrcStorOutput(const FullSubSel&);
				~SeisTrcStorOutput();

    GeomID			geomID() const
				{ return desiredsubsel_.geomID(0); }

    virtual bool		doInit();
    virtual void		set2D( bool yn = true )		{ is2d_ = yn; }
    virtual bool		useCoords(OD::GeomSystem) const	{ return false;}
    bool			getDesiredSubSel(FullSubSel&) const;
    bool			wantsOutput(const BinID&) const;
    virtual TypeSet< Interval<int> > getLocalZRanges(const BinID&,float,
						     TypeSet<float>&) const;
				mImplDefAttribOutputFns(Coord)
    bool			setStorageID(const DBKey&);
    void			setGeometry( const FullSubSel& fss )
				{ doSetGeometry(fss); }

    bool			doUsePar(const IOPar&,int outidx);
    virtual bool		finishWrite();
    virtual void		collectData(const DataHolder&,float step,
					    const SeisTrcInfo&);
    virtual bool		writeTrc();
    SeisTrc*			getTrc()		{ return trc_; }
    void			setTrcGrow( bool yn )	{ growtrctosi_ = yn; }
    void			setOutpTypes( const TypeSet<DataType>& typ )
				{ outptypes_ = typ; }
    void			setOutpNames( const BufferStringSet& nms )
				{ outpnames_ = nms; }

    virtual const uiString	errMsg() const
				{ return errmsg_; }

    static const char*		seisidkey();
    static const char*		attribkey();
    static const char*		inlrangekey();
    static const char*		crlrangekey();
    static const char*		depthrangekey();

protected:

    DBKey&			storid_;
    FullSubSel			desiredsubsel_;
    TypeSet< Interval<int> >	sampleinterval_;
    IOPar*			auxpars_;
    bool			is2d_;
    uiString			errmsg_;

    SeisTrc*			trc_;
    Seis::Storer*		storer_;
    BinID			prevpos_;
    bool			storinited_;
    BufferString		attribname_;
    Scaler*                     scaler_;
    TypeSet<DataType>		outptypes_;
    bool			growtrctosi_;
    BufferStringSet		outpnames_;

    float			writez0shift_;
    friend class		Processor;

public:

    void			deleteTrc(); //only if you do not use writeTrc

};


/*!
\brief 2D trace Output with variable Z range.
*/

mExpClass(AttributeEngine) Trc2DVarZStorOutput : public SeisTrcStorOutput
{ mODTextTranslationClass(Attrib::Trc2DVarZStorOutput)
public:
				Trc2DVarZStorOutput(Pos::GeomID,
						    DataPointSet*,float);

    bool			doInit();
    void			set2D(bool)			{}
    bool			useCoords(OD::GeomSystem) const	{ return true; }

    virtual bool		wantsOutput(const Coord&) const;
    TypeSet< Interval<int> >	getLocalZRanges(const Coord&,float,
						TypeSet<float>&) const;
				mImplDefAttribOutputFns(BinID)
    void			setTrcsBounds(Interval<float>);

    virtual bool		finishWrite();
    virtual void		collectData(const DataHolder&,float step,
					    const SeisTrcInfo&);
    void			setMaxDistBetwTrcs( float maxdist )
						{ maxdisttrcs_ = maxdist; }
protected:

    FullSubSel			getFSS();

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
    bool			wantsOutput(const BinID&) const;
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
						TypeSet<float>&) const;
				mImplDefAttribOutputFns(Coord)
    void			setGeometry(const Interval<int>&,
					    const Interval<float>&);
    bool			getDesiredSubSel(FullSubSel&) const;
    void			setOutput(Data2DHolder&);

    void			collectData(const DataHolder&,float step,
					    const SeisTrcInfo&);
    const uiString		errMsg() const	{ return errmsg_; }

protected:

    TypeSet< Interval<int> >	sampleinterval_;
    uiString		errmsg_;

    Data2DHolder*		output_;
};


/*!
\brief Output at discrete locations ( For example a pickset. )
*/

mExpClass(AttributeEngine) LocationOutput : public Output
{
public:
				LocationOutput(BinnedValueSet&);
				~LocationOutput() {};

    bool			getDesiredSubSel(FullSubSel&) const
				{ return true;}
    bool			wantsOutput(const BinID&) const;
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
						TypeSet<float>&) const;
				mImplDefAttribOutputFns(Coord)
    virtual void		collectData(const DataHolder&,float step,
					    const SeisTrcInfo&);
    void			setPossibleBinIDDuplic() { arebiddupl_ = true; }

    //TODO : check where we want to put this: output?
    static const char*		filenamekey();
    static const char*		locationkey();
    static const char*		attribkey();
    static const char*		surfidkey();

protected:
    BinnedValueSet&		bidvalset_;

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
				TrcSelectionOutput(const BinnedValueSet&,
						   float outval =0);
				~TrcSelectionOutput() ;

    bool			getDesiredSubSel(FullSubSel&) const;
    bool			wantsOutput(const BinID&) const;
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
						TypeSet<float>&) const;
				mImplDefAttribOutputFns(Coord)
    void			setOutput(SeisTrcBuf*);
    void			setTrcsBounds(Interval<float>);
    virtual void		collectData(const DataHolder&,float,
					    const SeisTrcInfo&);
    void			setGeomID(Pos::GeomID);

protected:
    const BinnedValueSet&	bidvalset_;
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
				~TableOutput() {};

    bool			getDesiredSubSel(FullSubSel&) const
				{ return true;}
    virtual bool		useCoords(OD::GeomSystem) const;
    bool			wantsOutput(const BinID&) const;
    bool			wantsOutput(const Coord&) const;
    virtual void		collectData(const DataHolder&,float step,
					    const SeisTrcInfo&);
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
						TypeSet<float>&) const;
    TypeSet< Interval<int> >	getLocalZRanges(const Coord&,float,
						TypeSet<float>&) const;

    void			setMaxDistBetwTrcs( float maxdist )
						{ maxdisttrcs_ = maxdist; }
    void			setPossibleBinIDDuplic() { arebiddupl_ = true; }

protected:
    DataPointSet&		datapointset_;

    bool			arebiddupl_;
    int				firstattrcol_;
    float			maxdisttrcs_;
    TypeSet<float>		distpicktrc_;

    void			computeAndSetVals(const DataHolder&,float,
						  float,float*);
    bool			areBIDDuplicated() const;
    void			addLocalInterval(TypeSet<Interval<int> >&,
						 TypeSet<float>&,
						 int,float) const;
};

} // namespace Attrib
