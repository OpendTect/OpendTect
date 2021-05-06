#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jul 2006
________________________________________________________________________

-*/

#include "generalmod.h"
#include "sets.h"
#include "executor.h"
#include "bufstringset.h"
#include "od_iosfwd.h"

namespace Table
{

mExpClass(General) ImportHandler
{
public:
			ImportHandler( od_istream& strm )
			    : strm_(strm)
			    , colpos_(0)	{}
    virtual		~ImportHandler()	{}

    enum State		{ Error, InCol, EndCol, EndRow };

    virtual State	add(char)		= 0;
    const char*		getCol() const		{ return col_.buf(); }
    const char*		errMsg() const		{ return col_.buf(); }

    virtual void	newRow()		{}
    virtual void	newCol()		{ col_.setEmpty(); colpos_ = 0;}

    char		readNewChar() const;
    bool		atEnd() const;

protected:

    od_istream&		strm_;
    BufferString	col_;
    int			colpos_;

    void		addToCol(char);

};


mExpClass(General) ExportHandler
{
public:
			ExportHandler( od_ostream& strm )
			    : strm_(strm)		{}
    virtual		~ExportHandler()		{}

    virtual bool	putRow(const BufferStringSet&,uiString&)	= 0;

    virtual bool	init();
    virtual void	finish();

    static bool		isNumber(const char*);

    BufferString	prepend_;
			//!< Before first record. Add newline if needed
    BufferString	append_;
			//!< After last record

protected:

    od_ostream&		strm_;

    uiString		getStrmMsg() const;

};



mExpClass(General) Converter : public Executor
{ mODTextTranslationClass(Converter);
public:
			Converter( ImportHandler& i, ExportHandler& o )
			    : Executor("Data import")
			    , imphndlr_(i), exphndlr_(o)
			    , rowsdone_(0), selcolnr_(-1), atend_(false)
			    , msg_(tr("Importing"))		{}
    // Setup
    TypeSet<int>	selcols_;
    uiString		msg_;

    virtual int		nextStep();
    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const	{ return tr("Records read"); }
    od_int64		nrDone() const		{ return rowsdone_; }

    struct RowManipulator
    {
	virtual bool	accept(BufferStringSet&) const		= 0;
			//!< if false returned, the row should not be written
    };
    void		setManipulator( const RowManipulator* m )
			    { manipulators_.erase(); addManipulator(m); }
    void		addManipulator( const RowManipulator* m )
			    { manipulators_ += m; }

protected:

    ImportHandler&	imphndlr_;
    ExportHandler&	exphndlr_;
    BufferStringSet	row_;
    ObjectSet<const RowManipulator> manipulators_;

    int			colnr_;
    int			selcolnr_;
    int			rowsdone_;
    bool		atend_;

    bool		handleImpState(ImportHandler::State);
    inline bool		colSel() const
			{ return selcols_.isEmpty()
			      || selcols_.isPresent(colnr_); }
};

}; // namespace Table


