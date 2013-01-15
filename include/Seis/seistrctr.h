#ifndef seistrctr_h
#define seistrctr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id$
________________________________________________________________________

Translators for seismic traces.

-*/

#include "seismod.h"
#include "transl.h"
#include "ctxtioobj.h"
#include "samplingdata.h"
#include "basiccompinfo.h"
#include "seisinfo.h"
#include "seistype.h"
#include "linekey.h"

class BinID;
class Coord;
class SeisTrc;
class LinScaler;
class SeisTrcBuf;
class SeisTrcInfo;
class CubeSampling;
class SeisPacketInfo;
class BufferStringSet;
namespace Seis		{ class SelData; }



/*!\brief Seismic Trace translator.

The protocol is as follows:

1) Client opens Connection apropriate for Translator. This connection will
   remain managed by client.

READ:

2) initRead() call initialises SeisPacketInfo, Component info and SelData
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


mClass(Seis) SeisTrcTranslatorGroup : public TranslatorGroup
{				isTranslatorGroup(SeisTrc)
public:
    			mDefEmptyTranslatorGroupConstructor(SeisTrc)
    
    static const char*	sKeyDefault3D() { return "Cube"; }
    static const char*	sKeyDefault2D()	{ return "LineSet"; }
    static const char*	sKeyDefaultAttrib() { return "Attribute"; }
    const char*		getSurveyDefaultKey(const IOObj*) const;
};


mClass(Seis) SeisTrcTranslator : public Translator
{
public:

    /*!\brief Information for one component

    This is info on data and how it is stored, where the 'store' can be on disk
    (read) or in memory (write).

    */

    mClass(Seis) ComponentData : public BasicComponentInfo
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

    mClass(Seis) TargetComponentData : public ComponentData
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
    Conn*		curConn()			{ return conn; }

    SeisPacketInfo&		packetInfo()		{ return pinfo; }
    const Seis::SelData*	selData() const		{ return seldata; }
    ObjectSet<TargetComponentData>& componentInfo()	{ return tarcds; }
    const SamplingData<float>&	inpSD() const		{ return insd; }
    int				inpNrSamples() const	{ return innrsamples; }
    const SamplingData<float>&	outSD() const		{ return outsd; }
    int				outNrSamples() const	{ return outnrsamples; }

    void		setSelData( const Seis::SelData* t ) { seldata = t; }
			/*!< This Seis::SelData is seen as a hint ... */
    bool		commitSelections();
			/*!< If not called, will be called by Translator.
			     For write, this will put tape header (if any) */

    virtual bool	readInfo(SeisTrcInfo&)		{ return false; }
    virtual bool	read(SeisTrc&)			{ return false; }
    virtual bool	skip( int nrtrcs=1 )		{ return false; }
    bool		write(const SeisTrc&);

    bool		close();
    const char*		errMsg() const			{ return errmsg; }

    virtual bool	inlCrlSorted() const		{ return true; }
    virtual int		bytesOverheadPerTrace() const	{ return 240; }
    virtual void	toSupported( DataCharacteristics& ) const {}
			//!< change the input to a supported characteristic

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
    bool		minimalHdrs() const		{ return false; }

    virtual void	cleanUp();
    			//!< Prepare for new initialisation.

    static bool		getRanges(const MultiID&,CubeSampling&,
	    			  const char* linekey=0);
    static bool		getRanges(const IOObj&,CubeSampling&,
	    			  const char* linekey=0);

    static bool		is2D(const IOObj&,bool only_internal=false);
    static bool		isPS(const IOObj&);
    bool		isPS() const			{ return is_prestack; }

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

    const LineKey&	curLineKey() const
			{ return curlinekey; }
    void		setCurLineKey( const LineKey& lk )
    			{ curlinekey = lk; }

    int			curGeomID() const		{ return geomid; }
    void		setCurGeomID( const int gid )	{ geomid = gid; }

    virtual bool	isReadDefault() const		{ return false; }
    virtual int		estimatedNrTraces() const	{ return -1; }

    void		setComponentNames(const BufferStringSet&);
    void		getComponentNames(BufferStringSet&) const;

    bool		haveWarnings() const;
    const BufferStringSet& warnings() const		{ return warnings_; }

protected:

    Conn*		conn;
    const char*		errmsg;
    SeisPacketInfo&	pinfo;
    BufferStringSet*	compnms_;

    Seis::ReadMode	read_mode;
    bool		is_2d;
    bool		is_prestack;
    bool		enforce_regular_write;
    bool		enforce_survinfo_write;

    SamplingData<float>			insd;
    int					innrsamples;
    ObjectSet<ComponentData>		cds;
    ObjectSet<TargetComponentData>	tarcds;
    const Seis::SelData*		seldata;
    SamplingData<float>			outsd;
    int					outnrsamples;
    Interval<int>			samps;
    LineKey				curlinekey;
    int					geomid;

    void		addComp(const DataCharacteristics&,
				const char* nm=0,int dtype=0);

    bool		initConn(Conn*,bool forread);
    void		setDataType( int icomp, int d )
			{ cds[icomp]->datatype = tarcds[icomp]->datatype = d; }

			/* Subclasses will need to implement the following: */
    virtual bool	initRead_()			{ return true; }
    virtual bool	initWrite_(const SeisTrc&)	{ return true; }
    virtual bool	commitSelections_()		{ return true; }
    virtual bool	prepareWriteBlock(StepInterval<int>&,bool&)
    							{ return true; }
    virtual bool	dumpBlock(); //!< will call blockDumped()
    virtual void	blockDumped(int nrtrcs)		{}
    void		prepareComponents(SeisTrc&,int actualsz) const;

			// Quick access to selected, like selComp() etc.
    ComponentData**	inpcds;
    TargetComponentData** outcds;

    			// Buffer written when writeBlock() is called
    SeisTrcBuf&		trcblock_;
    virtual bool	writeTrc_(const SeisTrc&)	{ return false; }

    TypeSet<int>	warnnrs_;
    BufferStringSet&	warnings_;
    virtual void	addWarn(int,const char*);

private:

    int*		inpfor_;
    int			nrout_;
    int			prevnr_;
    int			lastinlwritten;

    void		enforceBounds();
    bool		writeBlock();

public:

    void		setIs2D( bool yn )	{ is_2d = yn; }
    void		setIsPS( bool yn )	{ is_prestack = yn; }
};


#endif

