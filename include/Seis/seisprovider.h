#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

*/

#include "seistype.h"
#include "bin2d.h"
#include "datachar.h"
#include "dbkey.h"
#include "threadlock.h"
#include "zsubsel.h"

class BinnedValueSet;
class CubeHorSubSel;
class CubeSubSel;
class LineHorSubSel;
class LineSubSel;
class LineSubSelSet;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcInfo;
class SeisTrcTranslator;
class TraceData;
class TrcKey;
namespace Survey { class FullHorSubSel; class FullZSubSel; class FullSubSel; }
namespace PosInfo
{
    class CubeData; class LinesData;
    class LineData; class LineCollData;
}


namespace Seis
{

class ObjectSummary;
class Provider2D; class Provider3D;
class RawTrcsSequence;
class SelData;
class Fetcher; class Fetcher2D; class Fetcher3D;
class VolFetcher; class LineFetcher; class PS2DFetcher; class PS3DFetcher;


/*!\brief is the access point for seismic traces, from storage or DataPack.

 Instantiate a subclass and ask for what you need. The fetching can be done
 from multiple threads, but parameter setting has to be done before the start
 and is not MT-safe. There is a stand-alone goTo() which is MT-safe but can,
 by its nature, not be reliably used in MT condiditions. Then you have to
 reposition and get in one go, hence the 'getAt' functions.

 At or after instantiation, provide the DBKey with setInput. Then you can ask
 questions about the geometry and components available. You can also subselect
 the data. By default, you will get all stored components. If you want just one,
 use selectComponent().

 You can cross-get gathers as traces and vice versa. Get all components in a
 gather, or the entire gather in one trace as components. Just call the
 corresponding get function.

 If you need to know the exact positions and Z ranges after you have set
 SelData, Z extension and stepout, the use commitSelections(). Otherwise,
 you'll get possible positions and Z ranges.

 It is possible to obtain all possible positions, and all selected positions.
 You can also get the selected positions before the stepout is applied
 (the 'target' positions).

 Note: the getNext() function will return !isOK() at end of input. This will
 return the special 'error' 'Finished' which can be checked by
 isFinished( uirv ).

  */


mExpClass(Seis) Provider
{ mODTextTranslationClass(Seis::Provider);
public:

    mUseType( Pos,		GeomID );
    mUseType( Pos,		ZSubSel );
    mUseType( PosInfo,		LineCollData );
    mUseType( Survey,		FullHorSubSel );
    mUseType( Survey,		FullZSubSel );
    mUseType( Survey,		FullSubSel );
    typedef int			idx_type;
    typedef int			size_type;
    typedef float		z_type;
    typedef Interval<z_type>	z_rg_type;
    typedef od_int64		totsz_type;
    typedef idx_type		comp_idx_type;
    typedef TypeSet<comp_idx_type> comp_idx_set_type;
    typedef SeisTrcTranslator	STTrl;

    static Provider*	create(GeomType);
    static Provider*	create(const DBKey&,uiRetVal* uirv=0);
    static Provider*	create(const IOObj&,uiRetVal* uirv=0);
    static Provider*	create(const IOPar&,uiRetVal* uirv=0);
    static DBKey	dbKey(const IOPar&);

    virtual		~Provider();

    Provider2D*		as2D();
    const Provider2D*	as2D() const;
    Provider3D*		as3D();
    const Provider3D*	as3D() const;

    uiRetVal		setInput(const DBKey&);
    uiRetVal		setInput(const IOObj&);
    uiRetVal		reset()		{ return setInput(dbKey()); }

    GeomType		geomType() const { return geomTypeOf(is2D(),isPS()); }
    virtual bool	is2D() const	= 0;
    virtual bool	isPS() const	{ return false; }
    DBKey		dbKey() const;
    const IOObj*	ioObj() const	{ return ioobj_; }
    BufferString	name() const;

    const LineCollData&	possiblePositions() const { return possiblepositions_; }
    void		getComponentInfo(BufferStringSet&,DataType* dt=0) const;
    size_type		nrOffsets() const { return gtNrOffsets(); }
				//!< can vary; returned from a central location
    size_type		nrGeomIDs() const;
    GeomID		geomID(idx_type iln=0) const;
    const ZSubSel&	zSubSel(idx_type iln=0) const;
    ZSampling		zRange( idx_type iln=0 ) const
				{ return zSubSel(iln).outputZRange(); }
    void		getFullHorSubSel(FullHorSubSel&,
					 bool target=false) const;
    void		getFullZSubSel(FullZSubSel&) const;
    void		getFullSubSel(FullSubSel&,bool target=false) const;
    const BinnedValueSet* selectedPositions(bool target=false) const;

    void		setSelData(SelData*);		//!< Give it to me
    void		setSelData(const SelData&);	//!< I'll make a copy
    const SelData*	selData() const			{ return seldata_; }
    void		setZRange(const ZSampling&,idx_type iln=0);
			    //!< will be overruled if you set a SelData
    void		setZExtension(const z_rg_type&);
    void		selectComponent(comp_idx_type);
    void		selectComponents(const comp_idx_set_type&);
    void		forceFPData( bool yn=true ) { forcefpdata_ = yn; }
    void		setReadMode(ReadMode);
    const comp_idx_set_type& selectedComponents() const
						{ return selectedcomponents_; }
    bool		haveSelComps() const;
    DataCharacteristics	trcDataRep() const;

    void		commitSelections() const;
			    //!< will be done automatically if not called
			    //!< if you need to investigate the exact positions
			    //!< before you get traces, call this explicitly

    void		fillPar(IOPar&) const;
    uiRetVal		usePar(const IOPar&);

    bool		isEmpty() const;
    void		getPositions(LineCollData&,bool target=false) const;
    bool		isPresent(const TrcKey&) const;
    void		getCurPosition(TrcKey&) const;

    uiRetVal		getNext(SeisTrc&) const;
    uiRetVal		getNextGather(SeisTrcBuf&) const;
    uiRetVal		getNextSequence(RawTrcsSequence&) const;

    bool		goTo(const TrcKey&) const;
    uiRetVal		getCurrent(SeisTrc&) const;
    uiRetVal		getCurrentGather(SeisTrcBuf&) const;
    uiRetVal		getAt(const TrcKey&,SeisTrc&) const;
    uiRetVal		getGatherAt(const TrcKey&,SeisTrcBuf&) const;

    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr(bool target=false) const;

    static const char*	sKeyForceFPData()
			{ return "Force FPs"; }

protected:

    typedef TypeSet<ZSubSel> ZSubSelSet;
    typedef IJPos	SPos;
    enum WorkState	{ NeedInput, NeedPrep, Active };

			Provider(bool is2d,bool fillposs);

    IOObj*		ioobj_			= nullptr;
    ZSubSelSet		allzsubsels_;
    GeomIDSet		allgeomids_;
    LineCollData&	possiblepositions_;
    SelData*		seldata_		= nullptr;
    z_rg_type		zextension_		= z_rg_type(0.f,0.f);
    BinnedValueSet*	targetpositions_	= nullptr;
    BinnedValueSet*	selectedpositions_	= nullptr;
    ZSubSelSet		selectedzsubsels_;
    comp_idx_set_type	selectedcomponents_;
    ReadMode		readmode_		= Prod;
    bool		forcefpdata_		= false;

    mutable Threads::Lock getlock_;
    mutable WorkState	state_			= NeedInput;
    mutable SPos	spos_;
    mutable totsz_type	nrdone_			= 0;
    totsz_type		totalnr_		= -1;
    totsz_type		targettotalnr_		= -1;

    virtual Fetcher&	gtFetcher()		= 0;
    Fetcher&		fetcher()		{ return gtFetcher(); }
    const Fetcher&	fetcher() const		{ return mSelf().gtFetcher(); }

    void		reportSetupChg();
    void		handleNewPositions();
    void		ensureLineIdxOK(idx_type&) const;
    virtual size_type	gtNrOffsets() const			{ return 1; }
    ZSubSel&		possZSubSel(idx_type);
    void		prepWork(uiRetVal&);
    bool		prepGet(uiRetVal&,bool) const;
    bool		doGoTo(const TrcKey&) const;

    virtual void	gtTrc(TraceData&,SeisTrcInfo&,uiRetVal&) const	{}
    virtual void	gtGather(SeisTrcBuf&,uiRetVal&) const	{}

private:

    void		init(bool,bool);
    bool		prepareAccess(uiRetVal&) const;
    void		setTrcPos() const;
    void		wrapUpGet(TraceData&,uiRetVal&) const;
    void		wrapUpGet(SeisTrc&,uiRetVal&) const;
    void		wrapUpGet(SeisTrcBuf&,uiRetVal&) const;
    void		fillSequence(RawTrcsSequence&,uiRetVal&) const;
    uiRetVal		doGetTrc(SeisTrc&,bool next) const;
    uiRetVal		doGetGath(SeisTrcBuf&,bool next) const;

    void		getSelectedPositionsFromSelData();
    void		applyStepout();
    void		createSelectedZSubSels();

    friend class	Fetcher;
    friend class	Fetcher2D;
    friend class	Fetcher3D;
    friend class	VolFetcher;
    friend class	LineFetcher;
    friend class	PS2DFetcher;
    friend class	PS3DFetcher;

public:

    static void		putTraceInGather(const SeisTrc&,SeisTrcBuf&);
				//!< components become offsets 0, 100, 200, ...
    static void		putGatherInTrace(const SeisTrcBuf&,SeisTrc&);
				//!< offsets become components
    static void		getFallbackComponentInfo(BufferStringSet&,DataType&);

    const STTrl*	curTransl() const;

};


/*!\brief base class for Providers for 3D data. Extends Provider with some
  3D specific services. */


mExpClass(Seis) Provider3D : public Provider
{ mODTextTranslationClass(Seis::Provider3D);
public:

    mUseType( Pos,	IdxPair );
    mUseType( PosInfo,	CubeData );

    bool	is2D() const override	{ return false; }

    void	setStepout(const IdxPair&);

    const CubeData& possibleCubeData() const;
    bool	isPresent(const BinID&) const;
    BinID	curBinID() const	{ return trcpos_; }
    BinID	binIDStep() const;

    bool	goTo(const BinID&) const;

    uiRetVal	getAt(const BinID&,SeisTrc&) const;
    uiRetVal	getGatherAt(const BinID&,SeisTrcBuf&) const;

    static const Provider3D&	empty();
    static Provider3D&		dummy();

protected:

		Provider3D();
		Provider3D(const DBKey&,uiRetVal&);

    BinID	trcpos_;
    IdxPair	stepout_;

private:

    friend class	Provider;
    friend class	Fetcher;
    friend class	Fetcher3D;
    friend class	VolFetcher;
    friend class	PS3DFetcher;

};


/*!\brief base class for Providers for 2D data. Extends Provider with some
  2D specific services. */

mExpClass(Seis) Provider2D : public Provider
{ mODTextTranslationClass(Seis::Provider2D);
public:

    mUseType( PosInfo,	LineData );
    mUseType( PosInfo,	LinesData );
    mUseType( Bin2D,	trcnr_type );

    bool	is2D() const override	{ return true; }

    void	setStepout(trcnr_type); // use after setting seldata

    const LinesData& possibleLinesData() const;
    bool	isPresent(GeomID) const;
    bool	isPresent(const Bin2D&) const;
    size_type	nrLines() const;
    idx_type	lineIdx(GeomID) const;
    void	getLineData(idx_type,LineData&) const;
    trcnr_type	trcNrStep(idx_type) const;
    BufferString lineName( idx_type iln ) const	{ return geomID(iln).name(); }
    Bin2D	curBin2D() const		{ return trcpos_; }
    GeomID	curGeomID() const		{ return curBin2D().geomID(); }

    bool	goTo(const Bin2D&) const;
    uiRetVal	getAt(const Bin2D&,SeisTrc&) const;
    uiRetVal	getGatherAt(const Bin2D&,SeisTrcBuf&) const;

    static const Provider2D&	empty();
    static Provider2D&		dummy();

protected:


			Provider2D();
			Provider2D(const DBKey&,uiRetVal&);

    Bin2D		trcpos_;
    trcnr_type		stepout_	= 0;

    GeomID		gtGeomID(idx_type) const;
    ZSubSel&		gtPossZSubSel(idx_type) const;
    void		stZRange(idx_type,const ZSampling&);

private:

    friend class	Provider;
    friend class	Fetcher;
    friend class	Fetcher2D;
    friend class	LineFetcher;
    friend class	PS2DFetcher;

};

} // namespace Seis
