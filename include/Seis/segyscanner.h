#ifndef segyscanner_h
#define segyscanner_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segyscanner.h,v 1.6 2008-10-04 10:04:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "seistype.h"
#include "bufstringset.h"
#include "position.h"
#include "segyfiledef.h"
class IOPar;
class SeisTrc;
class DataPointSet;
class SEGYSeisTrcTranslator;


namespace SEGY
{
class FileData;


class Scanner : public Executor
{
public:

    			Scanner(const IOPar&,Seis::GeomType);
    			Scanner(const FileSpec&,Seis::GeomType,const IOPar&);
			~Scanner();

    Seis::GeomType	geomType() const	{ return geom_; }
    const IOPar&	pars() const		{ return pars_; }
    void		setMaxNrtraces( int n )	{ nrtrcs_ = n; }

    int			nextStep();
    const char*		message() const		{ return msg_.buf(); }
    od_int64		nrDone() const		{ return nrdone_; }
    const char*		nrDoneText() const	{ return "Traces scanned"; }

    const ObjectSet<FileData>& fileData() const	{ return fd_; }
    bool		toNext(SEGY::FileDef::TrcIdx&) const;

    BufferStringSet	fnms_;		//!< Actually used, possibly with errs
    BufferStringSet	failedfnms_;	//!< Failed to open or read
    BufferStringSet	failerrmsgs_;	//!< Err Msgs for failed
    BufferStringSet	scanerrfnms_;	//!< Error during scan (but in fnms_)
    BufferStringSet	scanerrmsgs_;	//!< Err Msgs for 'Error during scan'

    void		getReport(IOPar&) const;

protected:

    Seis::GeomType	geom_;
    const IOPar&	pars_;
    ObjectSet<FileData>	fd_;
    SEGYSeisTrcTranslator* tr_;
    int			nrtrcs_;

    SeisTrc&		trc_;
    int			curfidx_;
    BufferString	msg_;
    int			nrdone_;

    int			openNext();
    int			readNext();

    void		init(const FileSpec&);
    void		addFailed(const char*);
    void		initFileData();
    void		addErrReport(IOPar&) const;

};

} // namespace

#endif
