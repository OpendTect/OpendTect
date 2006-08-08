#ifndef tableconvimpl_h
#define tableconvimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id: tableconvimpl.h,v 1.6 2006-08-08 15:40:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "tableconv.h"
#include "bufstringset.h"


class CSVTableImportHandler : public TableImportHandler
{
public:
    			CSVTableImportHandler()
			    : nlreplace_('\n')
			    , instring_(false)	{}

    State		add(char);
    const char*		getCol() const		{ return col_.buf(); }
    const char*		errMsg() const		{ return col_.buf(); }

    virtual void	newRow()		{ instring_ = false; }

    char		nlreplace_;
    			//!< replace newlines with this char (optional)

protected:

    bool		instring_;

};


class CSVTableExportHandler : public TableExportHandler
{
public:

    const char*		putRow(const BufferStringSet&,std::ostream&);

protected:

    void		addVal(std::ostream&,int col,const char*);

};


class SQLInsertTableExportHandler : public TableExportHandler
{
public:

    			SQLInsertTableExportHandler()
			    : startindex_(1)
			    , stepindex_(1)
		    	    , nrrows_(0)	    {}

    const char*		putRow(const BufferStringSet&,std::ostream&);

    BufferString	tblname_;	//!< name of the table: mandatory
    BufferStringSet	colnms_;	//!< names of the columns: optional

    BufferString	indexcolnm_;	//!< if not empty, will add column
    int			startindex_;	//!< if indexcolnm_ set, startindex
    int			stepindex_;	//!< if indexcolnm_ set, step index

    BufferStringSet	extracolvals_;	//!< Values for columns not in input
    BufferStringSet	extracolnms_;	//!< Column names for extracolvals_

protected:

    void		addVal(std::ostream&,int col,const char*);

    int			nrrows_;
    bool		addindex_;
    int			nrextracols_;

};


class TCEmptyFieldRemover : public TableConverter::RowManipulator
{
public:
    			TCEmptyFieldRemover()		{}

    bool		accept(BufferStringSet&) const;

    TypeSet<int>	ckcols_; //!< column numbers

};


class TCDuplicateKeyRemover : public TableConverter::RowManipulator
{
public:
    			TCDuplicateKeyRemover()
			    : nrdone_(0), nrremoved_(0)	{}

    bool		accept(BufferStringSet&) const;

    TypeSet<int>	keycols_; //!< column numbers

    int			nrRemoved() const		{ return nrremoved_; }

protected:

    mutable BufferStringSet	prevkeys_;
    mutable int			nrdone_;
    mutable int			nrremoved_;

    void			setPrevKeys(const BufferStringSet&) const;

};


#endif
