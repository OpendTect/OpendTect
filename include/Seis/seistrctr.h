#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		10-5-1995
________________________________________________________________________

Translators for seismic traces.

-*/

#include "seisinfo.h"
#include "transl.h"
#include "ioobjctxt.h"
#include "samplingdata.h"
#include "basiccompinfo.h"
#include "tracedata.h"

class BufferStringSet;
class LinScaler;
class SeisPacketInfo;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcInfo;
class TrcKeyZSampling;
namespace PosInfo	{ class LineCollData; }
namespace Seis		{ class Provider; class SelData; }



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
   will fill default writing layout.
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
{   isTranslatorGroup(SeisTrc);
    mODTextTranslationClass(SeisTrcTranslatorGroup);
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

    mUseType( Seis,	DataType );
    mUseType( Seis,	ReadMode );
    mUseType( Seis,	SelData );
    mUseType( Pos,	GeomID );
    mUseType( PosInfo,	LineCollData );

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
    changed to desired values. If a component should not be read or written,
    set selected_ to false. If Translator returns true for isSingleComponent()
    then if you do not set selected_ flags yourself, then the first component
    is written.

    */

    mExpClass(Seis) TargetComponentData : public ComponentData
    {
	friend class	SeisTrcTranslator;

    public:

	bool		selected_;

    protected:

			TargetComponentData( const ComponentData& c )
			    : ComponentData(c), selected_(true) {}

	void		operator=(const TargetComponentData&)	= delete;

    };

			SeisTrcTranslator(const char*,const char*);
    virtual		~SeisTrcTranslator();

			/*! Init functions must be called, because
			     a Conn object must always be available */
    bool		initRead(Conn*,ReadMode rt=Seis::Prod);
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
    const SelData*		selData() const		{ return seldata_; }
    ObjectSet<TargetComponentData>& componentInfo()	{ return tarcds_; }
    const ObjectSet<TargetComponentData>&
				componentInfo() const	{ return tarcds_; }
    const ObjectSet<ComponentData>&
				inputComponentData() const { return cds_; }
    const SamplingData<float>&	inpSD() const		{ return insd_; }
    int				inpNrSamples() const	{ return innrsamples_; }
    const SamplingData<float>&	outSD() const		{ return outsd_; }
    int				outNrSamples() const	{ return outnrsamples_;}

    void		setSelData(const SelData*);
    bool		commitSelections();
			/*!< If not called, will be called by Translator.
			     For write, this will put tape header (if any) */

    virtual bool	readInfo(SeisTrcInfo&)		= 0;
    virtual bool	readData(TraceData* extbuf=0)	= 0;
    virtual bool	read(SeisTrc&);
    virtual bool	skip( int nrtrcs=1 )		{ return false; }
    virtual bool	supportsGoTo() const		{ return false; }
    virtual bool	goTo(const BinID&)		{ return false; }
    virtual bool	write(const SeisTrc&);
			// overrule if you don't need sorting/buffering

    uiString		errMsg() const			{ return errmsg_; }

    virtual bool	isSingleComponent() const	{ return true; }
    virtual bool	getGeometryInfo(LineCollData&) const
							{ return false; }
				//!< The returned CubeData should be sorted

    DataType		dataType() const		 { return datatype_; }
    void		setDataType( DataType dt )	{ datatype_ = dt; }
    virtual bool	isUserSelectable(bool) const	{ return false; }
    virtual int		bytesOverheadPerTrace() const	{ return 240; }
    virtual int		estimatedNrTraces() const	{ return -1; }

    virtual bool	close();
    virtual void	cleanUp();
			//!< Prepare for new initialisation.

    virtual void	usePar(const IOPar&);

    inline int		selComp( int nr=0 ) const	{ return inpfor_[nr]; }
    inline int		nrSelComps() const		{ return nrout_; }
    SeisTrc*		getEmpty();
			/*!< Returns an empty trace with the target data
				characteristics for component 0 */
    SeisTrc*		getFilled(const BinID&);
			/*!< Returns a full sized trace with zeros. */

    static bool		getRanges(const DBKey&,TrcKeyZSampling&,
				  const char* linekey=0);
    static bool		getRanges(const IOObj&,TrcKeyZSampling&,
				  const char* linekey=0);

    static bool		is2D(const IOObj&,bool only_internal=false);
    static bool		isPS(const IOObj&);
    bool		isPS() const			{ return is_prestack; }
    static bool		isLineSet(const IOObj&);
			/*!< Should only be used to filter out
			     old LineSet entries in .omf */

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

    GeomID		curGeomID() const		{ return geomid_; }
    void		setCurGeomID( GeomID gid )	{ geomid_ = gid; }

    void		setComponentNames(const BufferStringSet&);
    void		getComponentNames(BufferStringSet&) const;

    bool		haveWarnings() const;
    const uiStringSet&	warnings() const		{ return warnings_; }

    virtual bool	implRemove(const IOObj*) const;
    virtual bool	implRename(const IOObj*,const char*,
				   const CallBack* cb=0) const;
    virtual bool	implSetReadOnly(const IOObj*,bool yn) const;

    static BufferStringSet stdAuxExtensions();
    virtual BufferStringSet auxExtensions() const;

protected:

    Conn*		conn_;
    SeisPacketInfo&	pinfo_;
    uiString		errmsg_;
    BufferStringSet*	compnms_;

    ReadMode		read_mode;
    bool		is_2d;
    bool		is_prestack;
    bool		enforce_regular_write;
    bool		enforce_survinfo_write;

    SamplingData<float>			insd_;
    int					innrsamples_;
    ObjectSet<ComponentData>		cds_;
    ObjectSet<TargetComponentData>	tarcds_;
    const SelData*			seldata_;
    SamplingData<float>			outsd_;
    int					outnrsamples_;
    StepInterval<int>			samprg_;
    GeomID				geomid_;
    DataType				datatype_;
    bool				headerdone_;
    bool				datareaddone_;
    TraceData*				trcdata_;
    LinScaler*				trcscale_;
    const LinScaler*			curtrcscale_;

    int			nrSamps2Read() const
			{ return samprg_.stop-samprg_.start+1; }
    virtual bool	forRead() const;
    void		addComp(const DataCharacteristics&,const char* nm=0);

			/* Subclasses will need to implement the following: */
    virtual bool	initRead_()			{ return true; }
    virtual bool	initWrite_(const SeisTrc&)	{ return true; }
    virtual bool	commitSelections_()		{ return true; }
    virtual bool	wantBuffering() const		{ return true; }

			// These are called from the default write()
    virtual bool	prepareWriteBlock(StepInterval<int>&,bool&)
							{ return true; }
    virtual bool	dumpBlock(); //!< will call blockDumped()
    virtual void	blockDumped(int nrtrcs)		{}
    virtual bool	writeTrc_(const SeisTrc&)	{ return false; }
			// Buffer to be written when writeBlock() is called
    SeisTrcBuf&		trcblock_;

    bool		ensureSelectionsCommitted();
    void		prepareComponents(SeisTrc&,int actualsz) const;

			// Quick access to selected, like selComp() etc.
    ComponentData**	inpcds_;
    TargetComponentData** outcds_;

    TypeSet<int>	warnnrs_;
    uiStringSet		warnings_;
    virtual void	addWarn(int,const uiString& s=uiString::empty());

    virtual bool	removeMainObj(const IOObj&) const;
    virtual bool	renameMainObj(const IOObj&,const char*,
			      const CallBack* cb=0) const;
    virtual bool	setROMainObj(const IOObj&,bool) const;
    void		getAuxFileNames(const IOObj&,BufferStringSet&) const;

private:

    int*		inpfor_;
    int			nrout_;
    int			prevnr_;
    int			lastinlwritten_;

    bool		initConn(Conn*);
    void		enforceBounds();
    bool		writeBlock();

    friend class	Seis::Provider;

public:

    void		setIs2D( bool yn )	{ is_2d = yn; }
    void		setIsPS( bool yn )	{ is_prestack = yn; }
    mDeprecated virtual bool inlCrlSorted() const { return true; }

};
