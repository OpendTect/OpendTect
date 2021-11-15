#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Nov 2006
________________________________________________________________________

-*/

#include "seismod.h"

#include "bufstring.h"
#include "executor.h"
#include "seisstor.h"

class IOObj;
class Scaler;
class SeisTrc;
class SeisTrcBuf;
class BinIDSorting;
class SeisTrcReader;
class SeisTrcWriter;
class SeisResampler;
class BinIDSortingAnalyser;
namespace Seis { class SelData; }

/*!\brief Helps import or export of seismic data. */

mExpClass(Seis) SeisImporter : public Executor
{ mODTextTranslationClass(SeisImporter);
public:

    /*!<\brief provides traces from the import storage

      fetch() must return false at end or when an error occurs.
      On error, the errmsg_ must be filled.

      A SeisStdImporterReader based on SeisTrcReader is available.

      */

    mStruct(Seis) Reader
    {
	virtual			~Reader()			{}

	virtual const char*	name() const			= 0;
	virtual const char*	implName() const		= 0;
	virtual bool		fetch(SeisTrc&)			= 0;
	virtual int		totalNr() const			{ return -1; }

	uiString		errmsg_;
    };

			SeisImporter(Reader*,SeisTrcWriter&,Seis::GeomType);
				//!< Reader becomes mine. Has to be non-null.
    virtual		~SeisImporter();

    uiString		uiMessage() const override;
    od_int64		nrDone() const override;
    uiString		uiNrDoneText() const override;
    od_int64		totalNr() const override;

    int			nrSkipped() const	{ return nrskipped_; }
    Reader&		reader()		{ return *rdr_; }
    SeisTrcWriter&	writer()		{ return wrr_; }

    int			nextStep() override;

protected:

    enum State			{ ReadBuf, WriteBuf, ReadWrite };

    Reader*			rdr_;
    SeisTrcWriter&		wrr_;
    bool			writerismine_;
    int				queueid_;
    int				maxqueuesize_;
    Threads::ConditionVar&	lock_;
    SeisTrcBuf&			buf_;
    SeisTrc&			trc_;
    BinID&			prevbid_;
    int				sort2ddir_;
    BinIDSorting*		sorting_ = nullptr;
    BinIDSortingAnalyser*	sortanal_;
    Seis::GeomType		geomtype_;
    State			state_;
    int				nrread_ = 0;
    int				nrwritten_ = 0;
    int				nrskipped_ = 0;

    bool			goImpl(od_ostream*,bool,bool,int) override;

    bool			sortingOk(const SeisTrc&);
    int				doWrite(SeisTrc&);
    int				readIntoBuf();

    friend			class SeisImporterWriterTask;
    void			reportWrite(const uiString&);

    mutable uiString		errmsg_;
    mutable uiString	        hndlmsg_;

public:

};


mExpClass(Seis) SeisStdImporterReader : public SeisImporter::Reader
{ mODTextTranslationClass(SeisStdImporterReader);
public:
			SeisStdImporterReader(const SeisStoreAccess::Setup&,
					      const char* nm);
			~SeisStdImporterReader();

    const SeisTrcReader& reader() const		{ return rdr_; }
    SeisTrcReader&	reader()		{ return rdr_; }

    const char*		name() const		{ return name_; }
    const char*		implName() const;
    bool		fetch(SeisTrc&);

    void		removeNull( bool yn )	{ remnull_ = yn; }
    void		setResampler(SeisResampler*);	//!< becomes mine
    void		setScaler(Scaler*);		//!< becomes mine
    void		setSelData(Seis::SelData*);	//!< becomes mine

    int			totalNr() const;

protected:

    const BufferString	name_;
    SeisTrcReader&	rdr_;
    int			totalnr_ = -1;
    bool		remnull_ = false;
    SeisResampler*	resampler_ = nullptr;
    Scaler*		scaler_ = nullptr;

public:

    mDeprecated("Use SeisStoreAccess::Setup")
			SeisStdImporterReader(const IOObj&,const char* nm);

};


