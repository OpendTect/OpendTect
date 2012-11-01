#ifndef tableconv_h
#define tableconv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "sets.h"
#include "executor.h"
#include "bufstringset.h"
#include <iostream>

namespace Table
{

mClass(General) ImportHandler
{
public:
    			ImportHandler( std::istream& strm )
			    : strm_(strm)
			    , colpos_(0)	{}
    virtual		~ImportHandler()	{}

    enum State		{ Error, InCol, EndCol, EndRow };

    virtual State	add(char)		= 0;
    const char*		getCol() const		{ return col_.buf(); }
    const char*		errMsg() const		{ return col_.buf(); }

    virtual void	newRow()		{}
    virtual void	newCol()		{ *col_.buf() = '\0';
						  colpos_ = 0; }

    inline char		readNewChar() const
			{
			    char c = mCast( char, strm_.peek() );
			    strm_.ignore( 1 );
			    return atEnd() ? '\n' : c;
			}
    inline bool		atEnd() const		{ return strm_.eof(); }

protected:

    std::istream&	strm_;
    BufferString	col_;
    int			colpos_;

    void		addToCol(char);

};


mClass(General) ExportHandler
{
public:
    			ExportHandler( std::ostream& strm )
			    : strm_(strm)		{}
    virtual		~ExportHandler()		{}

    virtual const char*	putRow(const BufferStringSet&)	= 0;

    virtual bool	init();
    virtual void	finish();

    static bool		isNumber(const char*);

    BufferString	prepend_;
    			//!< Before first record. Add newline if needed
    BufferString	append_;
    			//!< After last record

protected:

    std::ostream&	strm_;

    inline const char*	getStrmMsg() const
			{ return strm_.good() ? 0 : "Error writing to output"; }

};



mClass(General) Converter : public Executor
{
public:
    			Converter( ImportHandler& i, ExportHandler& o )
			    : Executor("Data import")
			    , imphndlr_(i), exphndlr_(o)
			    , rowsdone_(0), selcolnr_(-1), atend_(false)
			    , msg_("Importing")		{}
    // Setup
    TypeSet<int>	selcols_;
    BufferString	msg_;

    virtual int		nextStep();
    const char*		message() const		{ return msg_.buf(); }
    const char*		nrDoneText() const	{ return "Records read"; }
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
			      || selcols_.indexOf(colnr_) > -1; }
};

}; // namespace Table


#endif

