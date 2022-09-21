#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "fontdata.h"
#include "namedobj.h"
#include "color.h"
#include "ranges.h"
#include "survinfo.h"

#include "bufstringset.h"


class uiWellDispPropDlg;
class uiWellPartServer;


namespace Well
{
class Data;
inline const char* sKey2DDispProp()	{ return "2D Display"; }
inline const char* sKey3DDispProp()	{ return "3D Display"; }

/*!
\brief Display properties of a well.
*/

mExpClass(Well) DisplayProperties
{
public:

			DisplayProperties(const char* subj = sKey3DDispProp());
			DisplayProperties(const DisplayProperties&);
    virtual		~DisplayProperties();

    DisplayProperties&	operator =(const DisplayProperties&);
    bool		operator ==(const DisplayProperties&) const;
    bool		operator !=(const DisplayProperties&) const;

    bool		is2D() const;
    bool		isValid() const		{ return isvalid_; }
    bool		isModified() const	{ return modified_; }

    mStruct(Well) BasicProps
    {
	virtual		~BasicProps();

	int		getSize() const			{ return size_; }
	const OD::Color& getColor() const		{ return color_; }

	void		setSize( int sz )		{ size_ = sz; }
	void		setColor( const OD::Color& col ) { color_ = col; }

	void		usePar(const IOPar&);
	void		fillPar(IOPar&) const;
	void		useLeftPar(const IOPar&);
	void		useCenterPar(const IOPar&);
	void		useRightPar(const IOPar&);
	void		fillLeftPar(IOPar&) const;
	void		fillCenterPar(IOPar&) const;
	void		fillRightPar(IOPar&) const;

	virtual const char* subjectName() const		= 0;

    protected:
			BasicProps(int sz);

	bool		operator ==(const BasicProps&) const;

	virtual void	doUsePar(const IOPar&)		{}
	virtual void	doFillPar(IOPar&) const		{}
	virtual void	doUseLeftPar(const IOPar&)	{}
	virtual void	doUseCenterPar(const IOPar&)	{}
	virtual void	doUseRightPar(const IOPar&)	{}
	virtual void	doFillLeftPar(IOPar&) const	{}
	virtual void	doFillCenterPar(IOPar&) const	{}
	virtual void	doFillRightPar(IOPar&) const	{}

	OD::Color	color_ = OD::Color(0,0,255);
	int		size_ = 1;

    };

    mStruct(Well) Track : public BasicProps
    {
			Track();
			Track(const Track&);
			~Track();

	const char*	subjectName() const override { return "Track"; }

	bool		operator ==(const Track&) const;
	bool		operator !=(const Track&) const;

	bool		dispabove_ = true;
	bool		dispbelow_ = true;
	bool		nmsizedynamic_ = false;
	FontData	font_ = 10;

    private:

	void		doUsePar(const IOPar&) override;
	void		doFillPar(IOPar&) const override;

    };

    mStruct(Well) Markers : public BasicProps
    {

			Markers();
			Markers(const Markers&);
			~Markers();

	Markers&	operator =(const Markers&);
	bool		operator ==(const Markers&) const;
	bool		operator !=(const Markers&) const;

	const char*	subjectName() const override { return "Markers"; }
	bool		isEmpty() const;
	bool		isSelected(const char* nm) const;

	const BufferStringSet& markerNms(bool issel) const;
	void		setMarkerNms(const BufferStringSet&,bool issel);

	int		shapeint_ = 0;
	int		cylinderheight_ = 1;
	bool		issinglecol_ = false;
	FontData	font_ = 10;
	OD::Color	nmcol_;
	bool		samenmcol_ = true;
	bool		nmsizedynamic_ = false;

    private:

	void		doUsePar(const IOPar&) override;
	void		doFillPar(IOPar&) const override;

	void		adjustSelection(const BufferStringSet& markernms);

	BufferStringSet selmarkernms_;
	BufferStringSet unselmarkernms_;

	friend class DisplayProperties;
    };

    mStruct(Well) Log : public BasicProps
    {
			Log();
			Log(const Log&);
			~Log();

	bool		operator ==(const Log&) const;
	bool		operator !=(const Log&) const;

	const char*	subjectName() const override { return "Log"; }
	void		setTo(const Data*,const Log&,bool forceifmissing=false);

	BufferString	name_ = sKey::None();
	BufferString	fillname_ = sKey::None();
	float           cliprate_ = 0.f;
	Interval<float> range_ = Interval<float>::udf();
	Interval<float> fillrange_ = Interval<float>::udf();
	bool		isleftfill_ = false;
	bool		isrightfill_ = false;
	bool		islogarithmic_ = false;
	bool		islogreverted_ = false;
	bool		issinglecol_ = false;
	bool		isdatarange_ = true;
	bool		iscoltabflipped_ = false;
	int		repeat_ = 5;
	float		repeatovlap_ = 50.f; //%
	OD::Color	linecolor_ = OD::Color::Red();
	OD::Color	seiscolor_ = OD::Color::White();
	BufferString    seqname_ = "Rainbow";
	int		logwidth_;
	int		style_ = 0;

    private:

	void		doUseLeftPar(const IOPar&) override;
	void		doUseCenterPar(const IOPar&) override;
	void		doUseRightPar(const IOPar&) override;
	void		doFillLeftPar(IOPar&) const override;
	void		doFillCenterPar(IOPar&) const override;
	void		doFillRightPar(IOPar&) const override;

    };

    mStruct(Well) LogCouple
    {
			LogCouple();
			LogCouple(const LogCouple&);
			~LogCouple();

	LogCouple&	operator =(const LogCouple&);
	bool		operator ==(const LogCouple&) const;
	bool		operator !=(const LogCouple&) const;

	Log left_, center_, right_;
    };

    void		setTrack(const Track&);
    void		setMarkers(const Data*,const Markers&);
    mDeprecated("Use setMarkerNames")
    void		setMarkersNms(const BufferStringSet&,bool issel);
    void		setMarkerNames(const BufferStringSet&,bool issel);
    void		setLeftLog(const Data*,const Log&,
				   int panelidx=0,bool forceifmissing=false);
    void		setCenterLog(const Data*,const Log&,
				     int panelidx=0,bool forceifmissing=false);
    void		setRightLog(const Data*,const Log&,
				    int panelidx=0,bool forceifmissing=false);
    void		setDisplayStrat(bool yn);

    const Track&	getTrack() const	{ return track_; }
    const Markers&	getMarkers() const	{ return markers_; }
    bool		displayStrat() const	{ return displaystrat_; }

    int			getNrLogPanels() const	{ return logs_.size(); }
    bool		isValidLogPanel(int idx) const;
    const LogCouple&	getLogs(int panel=0) const;

    void		ensureNrPanels(int);

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    void		ensureColorContrastWith(OD::Color);

    static DisplayProperties&	defaults();
    static void		commitDefaults();

    virtual const char* subjectName() const	{ return subjectname_.buf(); }

protected:

    BufferString	subjectname_;

private:

    bool		isvalid_ = false;
    bool		modified_ = false;

    Track		track_;
    Markers		markers_;
    ObjectSet<LogCouple> logs_;
    bool		displaystrat_ = false; //2d only

    void		setValid( bool yn )	{ isvalid_ = yn; }
    void		setModified( bool yn )	{ modified_ = yn; }
    friend class ::uiWellDispPropDlg;
    friend class ::uiWellPartServer;

};

} // namespace Well
