#ifndef tableconvimpl_h
#define tableconvimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id: tableconvimpl.h,v 1.3 2006-07-26 15:48:39 cvsbert Exp $
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
    			SQLInsertTableExportHandler();
			~SQLInsertTableExportHandler();

    const char*		putRow(const BufferStringSet&,std::ostream&);

    BufferString	tblname_;	//!< name of the table: mandatory
    BufferStringSet	colnms_;	//!< names of the columns: optional

protected:

    void		addVal(std::ostream&,int col,const char*);

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
