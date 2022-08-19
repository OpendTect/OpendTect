#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "tableconv.h"
#include "bufstringset.h"
#include "globexpr.h"
#include "od_iosfwd.h"
#include "uistring.h"


namespace Table
{

mExpClass(General) WSImportHandler : public ImportHandler
{
public:


			WSImportHandler( od_istream& s )
			    : ImportHandler(s)
			    , insingqstring_(false)
			    , indoubqstring_(false)	{}

    State		add(char) override;
    const char*		getCol() const			{ return col_.buf(); }
    const char*		errMsg() const			{ return col_.buf(); }

    void		newRow() override			{}

protected:

    bool		insingqstring_;
    bool		indoubqstring_;

};


mExpClass(General) CSVImportHandler : public ImportHandler
{
public:
			CSVImportHandler( od_istream& s )
			    : ImportHandler(s)
			    , nlreplace_('\n')
			    , instring_(false)	{}

    State		add(char) override;
    const char*		getCol() const		{ return col_.buf(); }
    const char*		errMsg() const		{ return col_.buf(); }

    void		newRow() override		{ instring_ = false; }

    char		nlreplace_;
			//!< replace newlines with this char (optional)

protected:

    bool		instring_;

};


mExpClass(General) WSExportHandler : public ExportHandler
{
public:

    enum ColWSHandl	{ None, Underscores, SingQuot, DoubQuot };

			WSExportHandler( od_ostream& s,
					 ColWSHandl w=Underscores )
			    : ExportHandler(s)
			    , colwshanld_(w)	{}

    bool		putRow(const BufferStringSet&,uiString&) override;

    ColWSHandl		colwshanld_;

protected:

    void		addVal(int col,const char*);

};


mExpClass(General) CSVExportHandler : public ExportHandler
{
public:
			CSVExportHandler( od_ostream& s )
			    : ExportHandler(s)		{}

    bool		putRow(const BufferStringSet&,uiString&) override;

protected:

    void		addVal(int col,const char*);

};


mExpClass(General) SQLInsertExportHandler : public ExportHandler
{ mODTextTranslationClass(SQLInsertExportHandler);
public:

			SQLInsertExportHandler( od_ostream& s )
			    : ExportHandler(s)
			    , startindex_(1)
			    , stepindex_(1)
			    , nrrows_(0)	    {}

    bool		putRow(const BufferStringSet&,uiString&) override;

    BufferString	tblname_;	//!< name of the table: mandatory
    BufferStringSet	colnms_;	//!< names of the columns: optional

    BufferString	indexcolnm_;	//!< if not empty, will add column
    int			startindex_;	//!< if indexcolnm_ set, startindex
    int			stepindex_;	//!< if indexcolnm_ set, step index

    BufferStringSet	extracolvals_;	//!< Values for columns not in input
    BufferStringSet	extracolnms_;	//!< Column names for extracolvals_

protected:

    void		addVal(int col,const char*);

    int			nrrows_;
    bool		addindex_;
    int			nrextracols_;

};


/*!\brief Removes lines at start or stop of input.

  Specify either a fixed number of lines, or an expression to match. For the
  stop_, the count is from the start and it determines how many records pass.

  */

mExpClass(General) StartStopManipulator : public Converter::RowManipulator
{
public:
		StartStopManipulator()
		    : startdone_(false)
		    , count_(0)		{}

    struct Criterion
    {
	enum Type	{ None, Records, Match };

			Criterion( Type t=None )
			    : type_(t)
			    , count_(1)
			    , matchcolidx_(0)		{}

	Type		type_;
	int		count_;		//!< nr of lines or nr of matches
	GlobExpr	matchexpr_;
	int		matchcolidx_;	//!< specify -1 for any col

    };

    Criterion	start_;
    Criterion	stop_;

    bool	accept(BufferStringSet&) const override;

protected:

    mutable bool startdone_;
    mutable int	 count_;

    void	updCount(const Criterion&,const BufferStringSet&) const;
    bool	isGEMatch(const Criterion&,const BufferStringSet&) const;

};


/*!\brief Only passes records where col(s) (don't) match expression(s) */


mExpClass(General) RecordMatcher : public Converter::RowManipulator
{
public:
			RecordMatcher( bool a=true )
			    : any_(a)	{}

    bool		accept(BufferStringSet&) const override;

    bool		any_;		//!< If false, all need to match
    bool		not_;		//!< If true, matches will not pass
    TypeSet<int>	ckcols_;	//!< Column numbers (mand)
    TypeSet<GlobExpr>	colvals_;	//!< Values associated (opt)
					//!< Not filled means check for empty

protected:

    static const GlobExpr emptyge_;

};


/*!\brief Removes records with identical keys

  Will only compare with previous record, so make sure the input is sorted
  on the keys.

  */


mExpClass(General) DuplicateKeyRemover : public Converter::RowManipulator
{
public:
			DuplicateKeyRemover()
			    : nrdone_(0), nrremoved_(0)	{}

    bool		accept(BufferStringSet&) const override;

    TypeSet<int>	keycols_; //!< column numbers (mand)

    int			nrRemoved() const		{ return nrremoved_; }

protected:

    mutable BufferStringSet	prevkeys_;
    mutable int			nrdone_;
    mutable int			nrremoved_;

    void			setPrevKeys(const BufferStringSet&) const;

};


mExpClass(General) FormatProvider
{
public:
			FormatProvider();
			~FormatProvider();

    const char*		xy() const;
    const char*		z(od_uint16 extradecimals=0) const;
    const char*		trcnr() const;
    const char*		spnr() const;
    const char*		string(od_uint16 length=10) const;

protected:
    void		readSettings();
};

} // namespace Table
