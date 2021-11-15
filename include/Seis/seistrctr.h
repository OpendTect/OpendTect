#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		10-5-1995
________________________________________________________________________

Translators for seismic traces.

-*/

#include "seismod.h"

#include "basiccompinfo.h"
#include "ctxtioobj.h"
#include "samplingdata.h"
#include "seisinfo.h"
#include "seistype.h"
#include "transl.h"

class BufferStringSet;
class LinScaler;
class SeisPacketInfo;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcInfo;
class TraceData;
class TrcKeyZSampling;

namespace Coords	{ class CoordSystem; }
namespace PosInfo	{ class CubeData; }
namespace Seis		{ class SelData; }


mDeclEmptyTranslatorBundle(Seis,SyntheticDataPars,dgb,"synthpars")


/*!\brief Seismic Trace translator.

The protocol is as follows:

1) Client opens Connection apropriate for Translator. This connection will
   remain managed by client.

READ:

2) initRead() call initializes SeisPacketInfo, Component info and SelData
   on input (if any)
3) Client subselects in components and space (SelData)
4) commitSelections()
5) By checking readInfo(), client may determine whether space selection
   was satisfied. Space selection is just a hint. This is done to protect
   client against (possible) freeze during (possible) search.
6) readData() reads actual trace components, or skip() skips trace(s).

WRITE:

2) with initWrite() client passes Connection and example trace. Translator
   will fill default writing layout. If Translator is single component,
   only the first component will have a destidx != -1.
3) Client sets trace selection and components as wanted
4) commitSelections() writes 'global' header, if any
5) write() writes selected traces/trace sections


lastly) close() finishes work (does not close connection). If you don't close
yourself, the destructor will do it but make sure it's done because otherwise
you'll likely loose an entire inline when writing.

Another note: the SelData type 'TrcNrs' is not supported by this class.
That is because of nasty implementation details on this level. The classes
SeisTrcReader and SeisTrcWriter do support it.

*/


mExpClass(Seis) SeisTrcTranslatorGroup : public TranslatorGroup
{				isTranslatorGroup(SeisTrc)
public:
			mDefEmptyTranslatorGroupConstructor(SeisTrc)

    static const char*	sKeyDefault3D() { return "Cube"; }
    static const char*	sKeyDefault2D()	{ return "2D Cube"; }
    static const char*	sKeyDefaultAttrib() { return "Attribute"; }
    const char*		getSurveyDefaultKey(const IOObj*) const;
};


mExpClass(Seis) SeisTrcTranslator : public Translator
{ mODTextTranslationClass(SeisTrcTranslator);
public:

    /*!\brief Information for one component

    This is info on data and how it is stored, where the 'store' can be on disk
    (read) or in memory (write).

    */

    mExpClass(Seis) ComponentData : public BasicComponentInfo
    {
	friend class	SeisTrcTranslator;

    protected:
			ComponentData( const char* nm="Seismic Data" )
			: BasicComponentInfo(nm)	{}
			ComponentData( const ComponentData& cd )
			: BasicComponentInfo(cd)	{}
			ComponentData(const SeisTrc&,int icomp=0,
				      const char* nm="Seismic Data");
	void		operator=(const ComponentData&);
			    //!< Protection against assignment.
    };


    /*!\brief ComponentData as it should be when the Translator puts it away.

    The data will be copied from the input ComponentData, but can then be
    changed to desired values. If a component should not be read/written,
    set destidx to -1.

    */

    mExpClass(Seis) TargetComponentData : public ComponentData
    {
	friend class	SeisTrcTranslator;

    public:

	int			destidx;
	const ComponentData&	org;

    protected:

			    TargetComponentData( const ComponentData& c,
						 int idx )
			    : ComponentData(c), org(c), destidx(idx)	{}

	void		operator=(const TargetComponentData&);
			    //!< Protection against assignment.
    };

			SeisTrcTranslator(const char*,const char*);
    virtual		~SeisTrcTranslator();

			/*! Init functions must be called, because
			     a Conn object must always be available */
    bool		initRead(Conn*,Seis::ReadMode rt=Seis::Prod);
			/*!< Conn* ptr will become mine, and it may be deleted
			  immediately!After call, component and packet info
			  will be available. Some STT's *require* a valid IOObj
			     in Conn. */
    bool		initWrite(Conn*,const SeisTrc&);
			/*!< Conn* ptr will become mine, and it may be deleted
			     immediately! After call, default component and
			     packet info will be generated according to the
			     example trace. Some STT's *require* a valid IOObj
			     in Conn */
    virtual Conn*	curConn()			{ return conn_; }

    SeisPacketInfo&		packetInfo()		{ return pinfo_; }
    const Seis::SelData*	selData() const		{ return seldata_; }
    ObjectSet<TargetComponentData>& componentInfo()	{ return tarcds_; }
    const ObjectSet<TargetComponentData>&
				componentInfo() const	{ return tarcds_; }
    const ObjectSet<ComponentData>&
				inputComponentData() const { return cds_; }
    const SamplingData<float>&	inpSD() const		{ return insd_; }
    int				inpNrSamples() const	{ return innrsamples_; }
    const SamplingData<float>&	outSD() const		{ return outsd_; }
    int				outNrSamples() const	{ return outnrsamples_;}

    void		setSelData( const Seis::SelData* t ) { seldata_ = t; }
			/*!< This Seis::SelData is seen as a hint ... */
    bool		commitSelections();
			/*!< If not called, will be called by Translator.
			     For write, this will put tape header (if any) */

    virtual bool	readInfo(SeisTrcInfo&)		{ return false; }
    virtual bool	read(SeisTrc&);
    virtual bool	skip( int nrtrcs=1 )		{ return false; }
    virtual bool	write(const SeisTrc&);
			// overrule if you don't need sorting/buffering

    virtual bool	close();
    uiString		errMsg() const			{ return errmsg_; }

    virtual bool	inlCrlSorted() const		{ return true; }
    virtual int		bytesOverheadPerTrace() const	{ return 240; }

    virtual void	usePar(const IOPar&);

    inline int		selComp( int nr=0 ) const	{ return inpfor_[nr]; }
    inline int		nrSelComps() const		{ return nrout_; }
    SeisTrc*		getEmpty();
			/*!< Returns an empty trace with the target data
				characteristics for component 0 */
    SeisTrc*		getFilled(const BinID&);
			/*!< Returns a full sized trace with zeros. */

    virtual bool	supportsGoTo() const		{ return false; }
    virtual bool	goTo(const BinID&)		{ return false; }

    virtual void	cleanUp();
			//!< Prepare for new initialisation.

    static bool		getRanges(const MultiID&,TrcKeyZSampling&,
				  const char* linekey=nullptr);
    static bool		getRanges(const IOObj&,TrcKeyZSampling&,
				  const char* linekey=nullptr);

    virtual bool	getGeometryInfo(PosInfo::CubeData&) const
							{ return false; }

    static bool		is2D(const IOObj&,bool only_internal=false);
    static bool		isPS(const IOObj&,bool only_internal=false);
    bool		is2D() const			{ return is_2d; }
    bool		isPS() const			{ return is_prestack; }
    static bool		isLineSet(const IOObj&);
			/*!< Should only be used to filter out
			     old LineSet entries in .omf */

    static const char*	sKeySeisTrPars();
    static const char*	sKeyIs2D();
    static const char*	sKeyIsPS();
    static const char*	sKeyRegWrite();
    static const char*	sKeySIWrite();

			// Use the following fns only if you _really_ know
			// what you're doing.
    void		enforceRegularWrite( bool yn )
			{ enforce_regular_write = yn; }
    void		enforceSurvinfoWrite( bool yn )
			{ enforce_survinfo_write = yn; }

    const char*		dataName() const	{ return dataname_.buf(); }
    void		setDataName( const char* nm )	{ dataname_ = nm; }
    Pos::GeomID		curGeomID() const		{ return geomid_; }
    virtual void	setCurGeomID(Pos::GeomID);

    virtual bool	isUserSelectable(bool) const	{ return false; }
    virtual int		estimatedNrTraces() const	{ return -1; }

    void		setComponentNames(const BufferStringSet&);
    void		getComponentNames(BufferStringSet&) const;

    bool		haveWarnings() const;
    const BufferStringSet& warnings() const		{ return warnings_; }

    const LinScaler*	traceScaler() const	{ return curtrcscalebase_; }

protected:

    Conn*		conn_ = nullptr;
    SeisPacketInfo&	pinfo_;
    uiString		errmsg_;
    BufferStringSet*	compnms_ = nullptr;

    Seis::ReadMode	read_mode = Seis::Prod;
    bool		is_2d = false;
    bool		is_prestack = false;
    bool		enforce_regular_write;
    bool		enforce_survinfo_write;

    SamplingData<float>			insd_;
    int					innrsamples_;
    ObjectSet<ComponentData>		cds_;
    ObjectSet<TargetComponentData>	tarcds_;
    const Seis::SelData*		seldata_ = nullptr;
    SamplingData<float>			outsd_;
    int					outnrsamples_;
    Interval<int>			samprg_;
    Pos::GeomID				geomid_;
    ConstRefMan<Coords::CoordSystem>	coordsys_;
    BufferString			dataname_;
    bool				headerdonenew_ = false;
    bool				datareaddone_ = false;
    TraceData*				storbuf_ = nullptr;
    LinScaler*				trcscalebase_ = nullptr;
    const LinScaler*			curtrcscalebase_ = nullptr;

    virtual bool	forRead() const;
    void		addComp(const DataCharacteristics&,
				const char* nm=0,int dtype=0);

    void		setDataType( int icomp, int d )
			{ cds_[icomp]->datatype = tarcds_[icomp]->datatype = d;}

			/* Subclasses will need to implement the following: */
    virtual bool	initRead_()			{ return true; }
    virtual bool	initWrite_(const SeisTrc&)	{ return true; }
    virtual bool	commitSelections_()		{ return true; }
    virtual bool	readData(TraceData* extbuf=0)	{ return false; }

			// These are called from the default write()
    virtual bool	prepareWriteBlock(StepInterval<int>&,bool&)
							{ return true; }
    virtual bool	dumpBlock(); //!< will call blockDumped()
    virtual void	blockDumped(int nrtrcs)		{}
    virtual bool	writeTrc_(const SeisTrc&)	{ return false; }
			// Buffer to be written when writeBlock() is called
    SeisTrcBuf&		trcblock_;

    void		prepareComponents(SeisTrc&,int actualsz) const;

			// Quick access to selected, like selComp() etc.
    ComponentData**	inpcds_ = nullptr;
    TargetComponentData** outcds_ = nullptr;

    TypeSet<int>	warnnrs_;
    BufferStringSet&	warnings_;
    virtual void	addWarn(int,const char*);

private:

    int*		inpfor_ = nullptr;
    int			nrout_ = 0;
    int			prevnr_ = mUdf(int);
    int			lastinlwritten_;

    bool		initConn(Conn*);
    void		enforceBounds();
    bool		writeBlock();
    bool		copyDataToTrace(SeisTrc&);

    friend class SeisTrcReader;

public:

    static void		setType(Seis::GeomType,IOPar&);
    static void		setGeomID(Pos::GeomID,IOPar&);
    static void		setCoordSys(const Coords::CoordSystem&,IOPar&);

    virtual void	setIs2D( bool yn )	{ is_2d = yn; }
    virtual void	setIsPS( bool yn )	{ is_prestack = yn; }
    virtual void	setCoordSys(const Coords::CoordSystem&);

    bool		readTraceData( TraceData* td=nullptr )
			{ return readData(td); }
};
