#ifndef seistrctr_h
#define seistrctr_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: seistrctr.h,v 1.4 2001-02-13 17:16:09 bert Exp $
________________________________________________________________________

Translators for seismic traces.

-*/

#include <transl.h>
#include <ctxtioobj.h>
#include <storlayout.h>
#include <datachar.h>
#include <samplingdata.h>
#include <seistype.h>

class Conn;
class SeisTrc;
class SeisTrcInfo;
class BinIDSelector;
class SeisTrcSel;
class SeisPacketInfo;



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
6) readData() reads actual trace components, or skip() skips trace.

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

    class ComponentData : public UserIDObject
    {
	friend class	SeisTrcTranslator;

    public:


	Seis::DataType		datatype;
	DataCharacteristics	datachar;
	SamplingData<float>	sd;
	int			nrsamples;

    protected:

			ComponentData( const char* nm="Seismic Data" )
			: UserIDObject(nm)
			, datatype(Seis::UnknowData)
			, sd(mUndefValue,mUndefValue)
			, nrsamples(0)			{}
			ComponentData( const ComponentData& cd )
			: UserIDObject(cd.name())
			, datatype(cd.datatype)
			, datachar(cd.datachar)
			, sd(cd.sd)
			, nrsamples(cd.nrsamples)	{}
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
			        ~TargetComponentData()			{}

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
			   be available. */
    bool		initWrite(Conn&,const SeisTrc&);
			/*!< After call, default component and packet info
			   will be generated according to the example trace. */

    SeisPacketInfo&			packetInfo()	{ return pinfo; }
    ObjectSet<TargetComponentData>&	componentInfo()	{ return tarcds; }
    const SeisTrcSel*			trcSel()	{ return trcsel; }
    void		setTrcSel( const SeisTrcSel* t ) { trcsel = t; }
			/*!< This SeisTrcSel is seen as a hint ... */

    bool		commitSelections();
			/*!< If not called, will be called by Translator.
			     For write, this will put tape header (if any) */

    virtual bool	readInfo(SeisTrcInfo&)		{ return false; }
    virtual bool	read(SeisTrc&)			{ return false; }
    virtual bool	skip()				{ return false; }
    virtual bool	write(const SeisTrc&)		{ return false; }

    virtual void	close()				{ conn = 0; }
    const char*		errMsg() const			{ return errmsg; }

    virtual StorageLayout storageLayout() const	
			{ return StorageLayout(StorageLayout::Inline,true); }

    virtual void	toSupported( DataCharacteristics& ) const {}
			//!< change the input to a supported characteristic
    virtual void	usePar(const IOPar*);

    static int		selector(const char*);
    static IOObjContext	ioContext();

    inline int		selComp( int nr=0 ) const	{ return inpfor_[nr]; }
    inline int		nrSelComps() const		{ return nrout_; }
    SeisTrc*		getEmpty();
			/*!< Returns an empty trace with the target data
				characteristics for component 0 */

protected:

    Conn*		conn;
    const char*		errmsg;
    SeisPacketInfo&	pinfo;

    const SeisTrcSel*			trcsel;
    ObjectSet<ComponentData>		cds;
    ObjectSet<TargetComponentData>	tarcds;

    void		addComp(const DataCharacteristics&,
				const SamplingData<float>&,int,
				const char* nm=0);

    bool		initConn(Conn&,bool forread);
    void		setDataType( int icomp, Seis::DataType d )
			{ cds[icomp]->datatype = tarcds[icomp]->datatype = d; }

    virtual void	cleanUp();
			/* Subclasses will need to implement the following: */
    virtual bool	initRead_()			{ return true; }
    virtual bool	initWrite_(const SeisTrc&)	{ return true; }
    virtual bool	commitSelections_()		{ return true; }

    IOPar&		storediopar;
    virtual void	useStoredPar();

private:

    int*		inpfor_;
    int			nrout_;

};


#endif
