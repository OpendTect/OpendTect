#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		June 2004
________________________________________________________________________

-*/

#include "seismod.h"
#include "seistrctr.h"
#include "executor.h"
#include "uistring.h"

class Seis2DDataSet;
class SeisTrc;
class SeisTrcBuf;
namespace PosInfo	{ class Line2DData; }
namespace Seis		{ class SelData; }


/*!\brief TranslatorGroup for 2D Seismic Data */

mExpClass(Seis) SeisTrc2DTranslatorGroup : public TranslatorGroup
{				isTranslatorGroup(SeisTrc2D)
public:
			mDefEmptyTranslatorGroupConstructor(SeisTrc2D)

    static const char*	sKeyDefault()	{ return "2D Cube"; }
    const char*		getSurveyDefaultKey(const IOObj*) const override;
};


/*!\brief interface for object that writes 2D seismic data */

mExpClass(Seis) Seis2DLinePutter
{ mODTextTranslationClass(Seis2DLinePutter);
public:
    virtual		~Seis2DLinePutter()	{}

    virtual bool	put(const SeisTrc&)	= 0;
    //!< Return fase on success, err msg on failure
    virtual bool	close()			= 0;
    //!< Return null on success, err msg on failure
    virtual uiString	errMsg() const		= 0;
    //!< Only when put or close returns false
    virtual int		nrWritten() const	= 0;
    virtual void	setComponentNames(const BufferStringSet&){};

};


/*!\brief interface for object that reads 2D seismic data */

mExpClass(Seis) Seis2DLineGetter : public Executor
{ mODTextTranslationClass(Seis2DLineGetter);
public:
    virtual		~Seis2DLineGetter()	{}

    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override
			{ return tr("Traces read"); }

    Pos::GeomID		geomID() const;
    bool		get(int trcnr,SeisTrc&) const;
    bool		get(int trcnr,TraceData&,SeisTrcInfo*) const;

    od_int64		nrDone() const override				= 0;
    od_int64		totalNr() const override			= 0;

    virtual const SeisTrcTranslator*	translator() const	{ return 0; }

protected:
			Seis2DLineGetter(SeisTrcBuf&,int trcsperstep,
					 const Seis::SelData&);

    int			nextStep() override				= 0;

    SeisTrcBuf&		tbuf_;
    uiString		msg_;
    Seis::SelData*	seldata_;
};

/*!\brief Provides read/write to/from 2D seismic lines.
	  Only interesting if you want to add your own 2D data I/O. */

mExpClass(Seis) Seis2DLineIOProvider
{ mODTextTranslationClass(Seis2DLineIOProvider);
public:

    virtual		~Seis2DLineIOProvider()			{}

    virtual bool	isEmpty(const IOObj&,Pos::GeomID) const		= 0;
    virtual bool	getGeomIDs(const IOObj&,TypeSet<Pos::GeomID>&) const
									= 0;
    virtual bool	getGeometry(const IOObj&,Pos::GeomID,
				    PosInfo::Line2DData&) const		= 0;
    virtual Executor*	getFetcher(const IOObj&,Pos::GeomID,SeisTrcBuf&,int,
				   const Seis::SelData* sd=0)		= 0;
    virtual Seis2DLinePutter* getPutter(const IOObj&,Pos::GeomID)	= 0;

    virtual bool	getTxtInfo(const IOObj&,Pos::GeomID,BufferString&,
				   BufferString&) const		{ return false;}
    virtual bool	getRanges(const IOObj&,Pos::GeomID,StepInterval<int>&,
				   StepInterval<float>&) const	{ return false;}

    virtual bool	removeImpl(const IOObj&,Pos::GeomID) const	= 0;
    virtual bool	renameImpl(const IOObj&,const char*) const	= 0;

    const char*		type() const			{ return type_.buf(); }

protected:

			Seis2DLineIOProvider( const char* t )
			: type_(t)				{}

    const BufferString	type_;
};


mGlobal(Seis) ObjectSet<Seis2DLineIOProvider>& S2DLIOPs();
//!< Sort of factory. Add a new type via this function.


//------
//! Translator mechanism is only used for selection etc.

mExpClass(Seis) TwoDSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(TwoDSeisTrcTranslator); isTranslator(TwoD,SeisTrc)
public:
			TwoDSeisTrcTranslator( const char* s1, const char* s2 )
			: SeisTrcTranslator(s1,s2)	{}

    const char*		defExtension() const override	{ return "2ds"; }
    bool		implRemove(const IOObj*) const override;
    bool		initRead_() override;	//!< supporting getRanges()
    bool		initWrite_(const SeisTrc&) override { return false; }

    bool		implRename( const IOObj*,const char*,
				    const CallBack* cb=0) const override;

};


/*!\brief Dummy old translator used during conversion only */
mExpClass(Seis) TwoDDataSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(TwoDDataSeisTrcTranslator);
  isTranslator(TwoDData,SeisTrc)
public:
			TwoDDataSeisTrcTranslator(const char* s1,const char* s2)
			: SeisTrcTranslator(s1,s2)	{}

};


/*!\brief Base translator for 2D Seismics */
mExpClass(Seis) SeisTrc2DTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(SeisTrc2DTranslator);
public:
			SeisTrc2DTranslator(const char* s1,const char* s2)
			    : SeisTrcTranslator(s1,s2)		{}

    bool		initRead_() override;	//!< supporting getRanges()
    bool		initWrite_(const SeisTrc&) override { return false; }

    bool		isUserSelectable(bool) const override	{ return true; }

    bool		implRemove(const IOObj*) const override;
    bool		implRename(const IOObj*,const char*,
				   const CallBack* =nullptr) const override;

    bool		getGeometryInfo(PosInfo::CubeData&) const override;

    void		setDataSet(const Seis2DDataSet&);

protected:

    const Seis2DDataSet*	dataset_ = nullptr;

};


/*!\brief CBVS translator for 2D Seismics */
mExpClass(Seis) CBVSSeisTrc2DTranslator : public SeisTrc2DTranslator
{ mODTextTranslationClass(CBVSSeisTrc2DTranslator);
  isTranslator(CBVS,SeisTrc2D)
public:
			CBVSSeisTrc2DTranslator(const char* s1,const char* s2)
			: SeisTrc2DTranslator(s1,s2)	{ setIs2D(true); }

    bool		isUserSelectable(bool) const override { return true; }
};

/*!\brief SEGYDirect translator for 2D Seismics */
mExpClass(Seis) SEGYDirectSeisTrc2DTranslator : public SeisTrc2DTranslator
{ mODTextTranslationClass(SEGYDirectSeisTrc2DTranslator);
  isTranslator(SEGYDirect,SeisTrc2D)
public:
			SEGYDirectSeisTrc2DTranslator(const char* s1,
						      const char* s2)
			: SeisTrc2DTranslator(s1,s2)	{ setIs2D(true); }

    bool		isUserSelectable(bool fr) const override { return fr; }
    const char*		iconName() const override	{ return "segy"; }
};


