#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    virtual		~ImportHandler();
			mOD_DisableCopy(ImportHandler)

    enum State		{ Error, InCol, EndCol, EndRow };

    virtual State	add(char)		= 0;
    const char*		getCol() const		{ return col_.buf(); }
    const char*		errMsg() const		{ return col_.buf(); }

    virtual void	newRow()		{}
    virtual void	newCol()		{ col_.setEmpty(); colpos_ = 0;}

    char		readNewChar() const;
    bool		atEnd() const;

protected:
			ImportHandler(od_istream&);

    od_istream&		strm_;
    BufferString	col_;
    int			colpos_		= 0;

    void		addToCol(char);

};


mExpClass(General) ExportHandler
{
public:
    virtual		~ExportHandler();
			mOD_DisableCopy(ExportHandler)

    virtual bool	putRow(const BufferStringSet&,uiString&)	= 0;

    virtual bool	init();
    virtual void	finish();

    static bool		isNumber(const char*);

    BufferString	prepend_;
			//!< Before first record. Add newline if needed
    BufferString	append_;
			//!< After last record

protected:
			ExportHandler(od_ostream&);

    od_ostream&		strm_;

    uiString		getStrmMsg() const;
};



mExpClass(General) Converter : public Executor
{ mODTextTranslationClass(Converter);
public:
			Converter(ImportHandler&,ExportHandler&);
			~Converter();
    // Setup
    TypeSet<int>	selcols_;
    uiString		msg_;

    int			nextStep() override;
    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override
			{ return tr("Records read"); }
    od_int64		nrDone() const override		{ return rowsdone_; }

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

    int			colnr_				= 0;
    int			selcolnr_			= -1;
    int			rowsdone_			= 0;
    bool		atend_				= false;

    bool		handleImpState(ImportHandler::State);
    inline bool		colSel() const
			{ return selcols_.isEmpty()
			      || selcols_.isPresent(colnr_); }
};

} // namespace Table
