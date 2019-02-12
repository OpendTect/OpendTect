#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2005
________________________________________________________________________

-*/

#include "generalmod.h"
#include "coltabmapper.h"
#include "datapackbase.h"
#include "draw.h"
#include "mousecursor.h"

class FlatView_CB_Rcvr;
class ZAxisTransform;
namespace ZDomain { class Def; }
namespace FlatView
{

typedef Geom::Point2D<double> Point;
typedef Geom::PosRectangle<double> Rect;

/*!
\brief Class that represents non-bitmap data to be displayed in a flatviewer,
such as markers, lines and more.
*/

mExpClass(General) AuxData
{
public:

    /*!\brief Explains what part of an AuxData's appearance may be edited by
    the user.*/

    mExpClass(General) EditPermissions
    {
    public:
			EditPermissions();

	bool		onoff_;
	bool		namepos_;
	bool		linestyle_;
	bool		linecolor_;
	bool		fillcolor_;
	bool		markerstyle_;
	bool		markercolor_;
	bool		x1rg_;
	bool		x2rg_;
    };


    virtual			~AuxData();
    virtual AuxData*		clone() const { return new AuxData(*this); }
    virtual void		touch()					{}

    EditPermissions*		editpermissions_;//!<If null no editing allowed

    bool			enabled_;	//!<Turns on/off everything
    bool			turnon_;	//!<Turns on/off display

    uiString			name_;
    OD::Alignment		namealignment_;
    int				namepos_;	//!<nodraw=udf, before first=-1,
					    //!< center=0, after last=1
    bool			fitnameinview_ = true;
    Interval<double>*		x1rg_;		//!<if 0, use viewer's rg & zoom
    Interval<double>*		x2rg_;		//!<if 0, use viewer's rg & zoom

    TypeSet<Point>		poly_;
    TypeSet<OD::MarkerStyle2D>	markerstyles_;
				/*!<- No markerstyles means no markers will be
				      displayed.
				    - If number of markerstyles is more than
				      zero, but less than number of points, the
				      last markerstyle will be used for the
				      excess points.
				*/

    OD::LineStyle		linestyle_;
    Color			fillcolor_;
    OD::FillPattern		fillpattern_;
    int				zvalue_; //!<overlay zvalue ( max=on top )
    MouseCursor			cursor_;

    bool			close_;
    bool			needsupdatelines_;

    void			setFillPattern( const OD::FillPattern& fp )
						{ fillpattern_ = fp; }
    bool			isEmpty() const;
    void			empty();

protected:
				AuxData(const char* nm);
				AuxData(const AuxData&);
				friend class Viewer;

    AuxData&			operator =(const AuxData&)	= delete;

};


/*!
\brief Annotation data for flatviewers.
*/

mExpClass(General) Annotation
{
public:


    /*!\brief Things like well tracks, cultural data, 2-D line positions.*/

    mStruct(General) AxisData
    {
				AxisData();

	uiString		name_;
	SamplingData<float>	sampling_;
	bool			showannot_;
	bool			showgridlines_;
	bool			reversed_;
	bool			annotinint_;
	int			factor_;
	bool			showauxannot_;
	uiString		auxlabel_;
	OD::LineStyle		auxlinestyle_;
	OD::LineStyle		auxhllinestyle_;
	TypeSet<OD::PlotAnnotation> auxannot_;
	int			auxPosIdx(float pos,float eps) const;
	void			showAll(bool yn);
    };


				Annotation(bool darkbg);
				~Annotation();

    uiString			title_; //!< color not settable
    Color			color_; //!< For axes
    AxisData			x1_;
    AxisData			x2_;

    bool			showaux_;
    bool			showscalebar_;
    bool			editable_;
    bool			allowuserchange_;
    bool			allowuserchangereversedaxis_;

    inline void		setAxesAnnot( bool yn ) //!< Convenience all or nothing
			{ x1_.showAll(yn); x2_.showAll(yn); }
    inline bool		haveTitle() const
			{ return !title_.isEmpty(); }
    inline bool		haveAxisAnnot( bool x1dir ) const
			{ return color_.isVisible()
			      && ( ( x1dir && x1_.showannot_)
				|| (!x1dir && x2_.showannot_)); }
    inline bool		haveGridLines( bool x1dir ) const
			{ return color_.isVisible()
			      && ( ( x1dir && x1_.showgridlines_)
				|| (!x1dir && x2_.showgridlines_)); }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static const char*	sKeyAxes();
    static const char*	sKeyX1Sampl();
    static const char*	sKeyX2Sampl();
    static const char*	sKeyShwAnnot();
    static const char*	sKeyShwGridLines();
    static const char*	sKeyIsRev();
    static const char*	sKeyShwAux();
};


/*!
\brief Data display parameters.

  When data needs to be displayed, there is a load of parameters and options
  for display. The two main display modes are:
  Variable Density = display according to color table
  Wiggle/Variable Area = wiggles with (possibly) filled amplitude
*/

mExpClass(General) DataDispPars
{
public:

    //!\brief Common to VD and WVA
    mExpClass(General) Common : public CallBacker
    {
    public:
			Common();
			Common(const Common&);
			~Common()		{ detachAllNotifiers(); }
	Common&		operator =(const Common&);

	bool		show_;	   // default=true
	bool		blocky_;   // default=false
	bool		allowuserchange_;
	bool		allowuserchangedata_;
	RefMan<ColTab::Mapper> mapper_; // do not set to null
    };

    //!\brief Variable Density (=color-bar driven) parameters
    mExpClass(General) VD : public Common
    {
    public:

			VD();

	BufferString	colseqname_;
	bool		lininterp_; // Use bi-linear interpol, not poly

    protected:

	void		tabNmChg(CallBacker*);

    };

    //!\brief Wiggle/Variable Area parameters
    mExpClass(General) WVA : public Common
    {
    public:

		    WVA()
			: wigg_(Color::Black())
			, refline_(Color::NoColor())
			, lowfill_(Color::NoColor())
			, highfill_(Color::Black())
			, overlap_(1)
			, reflinevalue_(mUdf(float))	{}

	Color		wigg_;
	Color		refline_;
	Color		lowfill_;
	Color		highfill_;
	float		overlap_;
	float		reflinevalue_;
    };

			DataDispPars()		{}

    VD			vd_;
    WVA			wva_;
    void		show( bool wva, bool vd )
			{ wva_.show_ = wva; vd_.show_ = vd; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static const char*	sKeyVD();
    static const char*	sKeyWVA();
    static const char*	sKeyShow();
    static const char*	sKeyDispRg();
    static const char*	sKeyColTab();
    static const char*  sKeyFlipSequence();
    static const char*  sKeyCyclicSequence();
    static const char*	sKeyLinearInter();
    static const char*	sKeyBlocky();
    static const char*  sKeyAutoScale();
    static const char*	sKeyClipPerc();
    static const char*	sKeyWiggCol();
    static const char*	sKeyRefLineCol();
    static const char*	sKeyLowFillCol();
    static const char*	sKeyHighFillCol();
    static const char*	sKeyOverlap();
    static const char*	sKeyRefLineValue();
};


/*!
\brief Flatviewer appearance.
*/

mExpClass(General) Appearance
{
public:
			Appearance( bool drkbg=true )
			    : darkbg_(drkbg)
			    , annot_(drkbg)
			    , secondsetaxes_(drkbg)
			    , anglewithset1_(0)		{}

    virtual		~Appearance()			{}

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    Annotation		annot_;
    DataDispPars	ddpars_;

    //Second set of axes, especially handy in case flat viewer is horizontal
    Annotation		secondsetaxes_;
    float		anglewithset1_;

    void		setDarkBG(bool yn);
    bool		darkBG() const		{ return darkbg_; }

    void		setGeoDefaults(bool isvert);

protected:

    bool		darkbg_;	//!< Two styles: dark (=black)
					//!< and lite (=white) background
					//!< Impacts a lot of display choices
};


/*!
\brief Flat Viewer using FlatView::Data and FlatView::Appearance.

  Interface for displaying data and related annotations where at least one of
  the directions is sampled regularly.

  The viewer works with FlatDataPacks - period. in previous versions, you could
  pass 'loose' data but DataPacks are so neat&clean we got rid of this
  possibility.

  You can attach many datapacks to the viewer; the different modes (WVA, VD)
  can be attached to zero or one of those packs.

  addPack() -> add a DataPack to the list of available data packs
  usePack() -> sets one of the available packs for wva of vd display
  setPack() -> Combination of addPack and usePack.
  removePack() -> removes this pack from the available packs, if necessary
		  it also clears the wva or vd display to no display.
*/

mExpClass(General) Viewer
{
public:

    typedef ::DataPack::ID  PackID;

			Viewer();
    virtual		~Viewer();

    virtual Appearance&	appearance();
    const Appearance&	appearance() const
			{ return const_cast<Viewer*>(this)->appearance(); }
    DataDispPars&	dispPars()	    { return appearance().ddpars_; }
    const DataDispPars&	dispPars() const    { return appearance().ddpars_; }

    ZAxisTransform*	getZAxisTransform() const
			{ return datatransform_; }
    bool		setZAxisTransform(ZAxisTransform*);
    bool		hasZAxisTransform() const
			{ return datatransform_; }
    void		setZDomain(const ZDomain::Def&);
    const ZDomain::Info& zDomain() const;

    void		addPack(PackID);
			/*!< Adds to list and obtains the DataPack, but does not
			 use for WVA or VD. DataPack gets released in the
			 destructor of this class. */
    void		usePack(bool wva,PackID,bool usedefs=true );
			//!< Does not add new packs, just selects from added
    void		removePack(PackID);
			//!< Releases DataPack after removing from the list.
    void		setPack( bool wva, PackID id, bool usedefs=true)
			{ addPack( id ); usePack( wva, id, usedefs ); }
    void		clearAllPacks();

    ConstRefMan<FlatDataPack>	getPack(bool wva,bool checkother=false) const;
			/*\param checkother if true, the datapack of other
			  display (i.e. variable density or wiggles) is returned
			  if the specified display has no datapack. */
    bool		hasPack(bool wva) const
			{ return packID(wva)!=DataPack::cNoID(); }
    PackID		packID(bool wva) const;
    const TypeSet<PackID>& availablePacks() const { return ids_; }
    bool		isAvailable( PackID id ) const
			{ return ids_.isPresent(id); }

    void		setMapper(bool wva,ColTab::Mapper&);
    RefMan<ColTab::Mapper> mapper(bool wva);
    ConstRefMan<ColTab::Mapper> mapper(bool wva) const;
    void		setMapperDistribFromDataPack(bool wva);

    virtual bool	isVertical() const		{ return true; }
    bool		isVisible(bool wva) const;
    void		setVisible(bool wva, bool visibility);
			//!< Will also handleChange.
			//!< So, do not use unless you want both.

    Coord3		getCoord(const Point&) const;

    enum DataChangeType	{ All=0xFFFFFFFF, BitmapData=0x0001, DisplayPars=0x0002,
			  Annot=0x0004, Auxdata=0x0008 };
    virtual void	handleChange( DataChangeType typ )
			{ doHandleChange( (unsigned int)typ ); }
    virtual void	handleChange( unsigned int dc ) // use bitwise or
			{ doHandleChange( dc ); }

			//!Does not store any data, just how data is displayed
    virtual void	fillAppearancePar( IOPar& iop ) const
			{ appearance().fillPar( iop ); }
			/*!Does not retrieve any data, just how data is
			  displayed */
    virtual void	useAppearancePar( const IOPar& iop )
			{ appearance().usePar( iop ); }

    void		storeDefaults(const char* key) const;
    void		useStoredDefaults(const char* key);

    void		getAuxInfo(const Point&,IOPar&) const;
    virtual void	showAuxDataObjects(AuxData&,bool)	{}
    virtual void	updateProperties(const AuxData&)	{}
    virtual void	reGenerate(AuxData&)		{}
    virtual void	remove(const AuxData&)		{}

    StepInterval<double> getDataPackRange(bool forx1) const;
    virtual Interval<float> getDataRange(bool wva) const;

    AuxData*		createAuxData( const char* nm ) const
			{ return doCreateAuxData( nm ); }
    int			nrAuxData() const	    { return gtNrAuxData(); }
    AuxData*		getAuxData( int idx )	    { return gtAuxData( idx ); }
    const AuxData*	getAuxData( int idx ) const { return gtAuxData( idx ); }
    void		addAuxData( AuxData* a )    { doAddAuxData( a ); }
    AuxData*		removeAuxData( AuxData* a )
			{ return doRemoveAuxData( a ); }
    AuxData*		removeAuxData( int idx )
			{ return doRemoveAuxDataByIdx( idx ); }

    void		removeAuxDatas(ObjectSet<AuxData>&);
    void		removeAllAuxData();
    virtual void	setAnnotChoice(int selannot)		{}
    virtual int		getAnnotChoice(BufferStringSet&) const { return -1; }
    void		enableStatusBarUpdate()  { needstatusbarupd_ = true; }
    void		disableStatusBarUpdate() { needstatusbarupd_ = false; }
    bool		needStatusBarUpdate() const { return needstatusbarupd_;}
    void		setSeisGeomidsToViewer(GeomIDSet&);

    const GeomIDSet&	getAllSeisGeomids() const { return geom2dids_; }

protected:

    TypeSet<PackID>	ids_;
    Appearance*		defapp_;
    DataPackMgr&	dpm_;
    ZAxisTransform*	datatransform_;
    ZDomain::Info*	zdinfo_;
    FlatView_CB_Rcvr*	cbrcvr_;
    mutable Threads::Lock lock_;
    bool		needstatusbarupd_;

    void		addAuxInfo(bool,const Point&,IOPar&) const;

    virtual void	doHandleChange(unsigned int)		= 0;
    virtual AuxData*	doCreateAuxData(const char* nm) const	= 0;
    virtual int		gtNrAuxData() const			= 0;
    virtual AuxData*	gtAuxData(int) const			= 0;
    virtual void	doAddAuxData(AuxData*)			= 0;
    virtual AuxData*	doRemoveAuxData(AuxData*)		= 0;
    virtual AuxData*	doRemoveAuxDataByIdx(int)		= 0;

private:

    WeakPtr<FlatDataPack>	wvapack_;
    WeakPtr<FlatDataPack>	vdpack_;

    GeomIDSet			geom2dids_;

public:

    mDeprecated const FlatDataPack* obtainPack(bool wva,
					bool checkother=false) const;

};

} // namespace FlatView
