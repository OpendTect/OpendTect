#ifndef seistrctr_h
#define seistrctr_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: seistrctr.h,v 1.15 2001-10-18 16:21:45 bert Exp $
________________________________________________________________________

Translators for seismic traces.

-*/

#include <transl.h>
#include <ctxtioobj.h>
#include <storlayout.h>
#include <basiccompinfo.h>
#include <streamconn.h>

class SeisTrc;
class SeisTrcInfo;
class BinID;
class BinIDSelector;
class SeisTrcSel;
class SeisPacketInfo;
class LinScaler;



/*!\brief Seismic Trace translator.

The protocol is as follows:

1) Client opens Connection apropriate for Translator. This connection will
   remain managed by client.

READ:

2) initRead() call initialises SeisPacketInfo, Component info and SeisTrcSel
   on input (if any)
3) Client subselects in components and space (SeisTrcSel)
4) commitSelections()
5) By checking readInfo(), client may determine whether space selection
   was satisfied. Space selection is just a hint. This is done to protect
   client against (possible) freeze during (possible) search.
6) readData() reads actual trace components, or skip() skips trace(s).

WRITE:

2) with initWrite() client passes Connection and example trace. Translator
   will fill default writing layout. If Translator is single component,
   only the first component will have a destidx != -1.
3) Client sets BinIDSelector and Components as wanted
4) commitSelections() writes 'global' header, if any
5) write() writes selected traces/trace sections


6/7) Finally, close() finishes work (does not close connection).

*/


class SeisTrcTranslator : public Translator
{			  isTranslatorGroup(SeisTrc)
public:

    /*!\brief Information for one component

    This is info on data and how it is stored, where the 'store' can be on disk
    (read) or in memory (write).

    */

    class ComponentData : public BasicComponentInfo
    {
	friend class	SeisTrcTranslator;

    protected:
			ComponentData( const char* nm="Seismic Data" )
			: BasicComponentInfo(nm)
			{ sd = SamplingData<float>(mUndefValue,mUndefValue); }
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

    class TargetComponentData : public ComponentData
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


			SeisTrcTranslator(const char* nm=0,
			      const ClassDef* cd=&StreamConn::classdef);
    virtual		~SeisTrcTranslator();

			/*! Init functions must be called, because
			     Conn object must be always available */
    bool		initRead(Conn&);
			/*!< After call, component and packet info will
			   be available. Note that Conn MUST have IOObj* */
    bool		initWrite(Conn&,const SeisTrc&);
			/*!< After call, default component and packet info
			   will be generated according to the example trace.
			   Note that Conn MUST have IOObj* */

    SeisPacketInfo&			packetInfo()	{ return pinfo; }
    const SeisTrcSel*			trcSel()	{ return trcsel; }
    ObjectSet<TargetComponentData>&	componentInfo()	{ return tarcds; }
    void				useInputSampling( bool yn=true )
					{ useinpsd = yn; }

    void		setTrcSel( const SeisTrcSel* t ) { trcsel = t; }
			/*!< This SeisTrcSel is seen as a hint ... */
    bool		commitSelections(const SeisTrc* trc=0);
			/*!< If not called, will be called by Translator.
			     For write, this will put tape header (if any) */

    virtual bool	readInfo(SeisTrcInfo&)		{ return false; }
    virtual bool	read(SeisTrc&)			{ return false; }
    virtual bool	skip( int nrtrcs=1 )		{ return false; }
    virtual bool	write(const SeisTrc&)		{ return false; }

    virtual void	close()				{ conn = 0; }
    const char*		errMsg() const			{ return errmsg; }

    virtual StorageLayout storageLayout() const	
			{ return StorageLayout(StorageLayout::Inline,true); }

    virtual void	toSupported( DataCharacteristics& ) const {}
			//!< change the input to a supported characteristic
    virtual void	usePar(const IOPar*);

    inline int		selComp( int nr=0 ) const	{ return inpfor_[nr]; }
    inline int		nrSelComps() const		{ return nrout_; }
    SeisTrc*		getEmpty();
			/*!< Returns an empty trace with the target data
				characteristics for component 0 */

    virtual bool	supportsGoTo() const		{ return false; }
    virtual bool	goTo(const BinID&)		{ return false; }
    void		forceWriteIntegrity(bool)	{}

    static int		selector(const char*);
    static const IOObjContext&	ioContext();

protected:

    Conn*		conn;
    const char*		errmsg;
    SeisPacketInfo&	pinfo;
    bool		useinpsd;

    const SeisTrcSel*			trcsel;
    ObjectSet<ComponentData>		cds;
    ObjectSet<TargetComponentData>	tarcds;

    void		addComp(const DataCharacteristics&,
				const SamplingData<float>&,int,
				const char* nm=0,const LinScaler* =0,
				int dtype=0);

    bool		initConn(Conn&,bool forread);
    void		setDataType( int icomp, int d )
			{ cds[icomp]->datatype = tarcds[icomp]->datatype = d; }

    virtual void	cleanUp();
			/* Subclasses will need to implement the following: */
    virtual bool	initRead_()			{ return true; }
    virtual bool	initWrite_(const SeisTrc&)	{ return true; }
    virtual bool	commitSelections_()		{ return true; }

    IOPar&		storediopar;
    virtual void	useStoredPar();

    void		prepareComponents(SeisTrc&,int* actualsize) const;

			// Quick access to selected, like selComp() etc.
    ComponentData**	inpcds;
    TargetComponentData** outcds;

private:

    int*		inpfor_;
    int			nrout_;

};


#endif
