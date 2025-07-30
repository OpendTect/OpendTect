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
			WSImportHandler(od_istream&);
			~WSImportHandler();

    State		add(char) override;
    const char*		getCol() const			{ return col_.buf(); }
    const char*		errMsg() const			{ return col_.buf(); }

    void		newRow() override		{}

    char		nlreplace_			= ' ';
			/*!< replace newlines found in fields
			     with this char (optional) */

private:
    bool		insingqstring_	= false;
    bool		indoubqstring_	= false;

};


mExpClass(General) CSVImportHandler : public ImportHandler
{
public:
			CSVImportHandler(od_istream&);
			~CSVImportHandler();

    State		add(char) override;
    const char*		getCol() const		{ return col_.buf(); }
    const char*		errMsg() const		{ return col_.buf(); }

    void		newRow() override	{ instring_ = false; }

    char		nlreplace_		= ' ';
			/*!< replace newlines found in fields
			     with this char (optional) */
private:
    bool		instring_ = false;

};


mExpClass(General) WSExportHandler : public ExportHandler
{
public:

    enum ColWSHandl	{ None, Underscores, SingQuot, DoubQuot };

			WSExportHandler(od_ostream&,ColWSHandl w=Underscores);
			~WSExportHandler();

    bool		putRow(const BufferStringSet&,uiString&) override;

    ColWSHandl		colwshanld_;

private:
    void		addVal(int col,const char*);

};


mExpClass(General) CSVExportHandler : public ExportHandler
{
public:
			CSVExportHandler(od_ostream&);
			~CSVExportHandler();

    bool		putRow(const BufferStringSet&,uiString&) override;

private:
    void		addVal(int col,const char*);

};


mExpClass(General) SQLInsertExportHandler : public ExportHandler
{ mODTextTranslationClass(SQLInsertExportHandler);
public:

			SQLInsertExportHandler(od_ostream&);
			~SQLInsertExportHandler();

    bool		putRow(const BufferStringSet&,uiString&) override;

    BufferString	tblname_;	//!< name of the table: mandatory
    BufferStringSet	colnms_;	//!< names of the columns: optional

    BufferString	indexcolnm_;	//!< if not empty, will add column
    int			startindex_ = 1;//!< if indexcolnm_ set, startindex
    int			stepindex_  = 1;//!< if indexcolnm_ set, step index

    BufferStringSet	extracolvals_;	//!< Values for columns not in input
    BufferStringSet	extracolnms_;	//!< Column names for extracolvals_

private:

    void		addVal(int col,const char*);

    int			nrrows_ = 0;
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
				StartStopManipulator();
				~StartStopManipulator();

    struct Criterion
    {
	enum Type	{ None, Records, Match };

			Criterion(Type=None);
			~Criterion();

	Type		type_;
	int		count_ = 1;	//!< nr of lines or nr of matches
	GlobExpr	matchexpr_;
	int		matchcolidx_ = 0;	//!< specify -1 for any col

    };

    Criterion	start_;
    Criterion	stop_;

private:

    mutable bool startdone_ = false;
    mutable int	 count_ = 0;

    void	updCount(const Criterion&,const BufferStringSet&) const;
    bool	isGEMatch(const Criterion&,const BufferStringSet&) const;
    bool	accept(BufferStringSet&) const override;

};


/*!\brief Only passes records where col(s) (don't) match expression(s) */


mExpClass(General) RecordMatcher : public Converter::RowManipulator
{
public:
			RecordMatcher(bool a=true);
			~RecordMatcher();

private:

    bool		accept(BufferStringSet&) const override;

    bool		any_;		//!< If false, all need to match
    bool		not_;		//!< If true, matches will not pass
    TypeSet<int>	ckcols_;	//!< Column numbers (mand)
    TypeSet<GlobExpr>	colvals_;	//!< Values associated (opt)
					//!< Not filled means check for empty
    static const GlobExpr emptyge_;

};


/*!\brief Removes records with identical keys

  Will only compare with previous record, so make sure the input is sorted
  on the keys.

  */


mExpClass(General) DuplicateKeyRemover : public Converter::RowManipulator
{
public:
			DuplicateKeyRemover();
			~DuplicateKeyRemover();

private:

    int			nrRemoved() const		{ return nrremoved_; }
    void		setPrevKeys(const BufferStringSet&) const;
    bool		accept(BufferStringSet&) const override;

    TypeSet<int>	keycols_; //!< column numbers (mand)
    mutable BufferStringSet	prevkeys_;
    mutable int			nrdone_ = 0;
    mutable int			nrremoved_ = 0;

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
