#ifndef tableconv_h
#define tableconv_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id: tableconv.h,v 1.1 2006-07-26 08:32:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "executor.h"
#include <iostream>


class TableImportHandler
{
public:
    			TableImportHandler()
			    : colpos_(0)	{}

    enum State		{ Error, InCol, EndCol, EndRow };

    virtual State	add(char)		= 0;
    const char*		getCol() const		{ return col_.buf(); }
    const char*		errMsg() const		{ return col_.buf(); }

    virtual void	newRow()		{}
    virtual void	newCol()		{ *col_.buf() = '\0';
						  colpos_ = 0; }
protected:

    void		addToCol(char);

    BufferString	col_;
    int			colpos_;

};


class TableExportHandler
{
public:

    virtual const char*	useColVal(int col,const char*)	= 0;
    virtual const char*	putRow(std::ostream&)		= 0;

    static bool		isNumber(const char*);
};



class TableConverter : public Executor
{
public:
    			TableConverter( std::istream& is, TableImportHandler& i,
					std::ostream& os, TableExportHandler& o)
			    : Executor("Data import")
			    , istrm_(is), ostrm_(os)
			    , imphndlr_(i), exphndlr_(o)
			    , rowsdone_(0)
			    , msg_("Importing")		{}
    // Setup
    TypeSet<int>	selcols_;
    BufferString	msg_;

    virtual int		nextStep();
    const char*		message() const		{ return msg_.buf(); }
    const char*		nrDoneText() const	{ return "Records read"; }
    int			nrDone() const		{ return rowsdone_; }

protected:

    std::istream&	istrm_;
    std::ostream&	ostrm_;
    TableImportHandler&	imphndlr_;
    TableExportHandler&	exphndlr_;

    int			colnr_;
    int			selcolnr_;
    int			rowsdone_;

    bool		handleImpState(TableImportHandler::State);
    bool		colSel() const
			{ return selcols_.size() < 1
			      || selcols_.indexOf(colnr_) > -1; }
    char		readNewChar() const
			{
			    char c = istrm_.peek();
			    istrm_.ignore( 1 );
			    return c;
			}
};


#endif
