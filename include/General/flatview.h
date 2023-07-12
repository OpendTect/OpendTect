#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			EditPermissions() {}

	bool		onoff_ = true;
	bool		namepos_ = true;
	bool		linestyle_ = true;
	bool		linecolor_ = true;
	bool		fillcolor_ = true;
	bool		markerstyle_ = true;
	bool		markercolor_ = true;
	bool		x1rg_ = true;
	bool		x2rg_ = true;
    };


    virtual			~AuxData();
    virtual AuxData*		clone() const { return new AuxData(*this); }
    virtual void		touch()					{}

    EditPermissions*		editpermissions_ = nullptr;
				//!<If null no editing allowed

    bool			enabled_ = true;
				//!<Turns on/off everything
    BufferString		name_;
    Alignment			namealignment_ = mAlignment(Center,Center);
    enum NamePos		{ First=-1, Center=0, Last=1, NoDraw };
    NamePos			namepos_ = NoDraw;

    bool			fitnameinview_ = true;
    Interval<double>*		x1rg_ = nullptr;
    Interval<double>*		x2rg_ = nullptr;
				//!<if null, use viewer's rg & zoom

    TypeSet<Point>		poly_;
    TypeSet<MarkerStyle2D>	markerstyles_;
				/*!<- No markerstyles means no markers will be
				      displayed.
				    - If number of markerstyles is more than
				      zero, but less than number of points, the
				      last markerstyle will be used for the
				      excess points.
				*/

    OD::LineStyle		linestyle_;
    OD::Color			fillcolor_ = OD::Color::NoColor();
    FillPattern			fillpattern_;

    mExpClass(General) FillGradient
    {
    public:
			FillGradient();
			~FillGradient();

	void		set(const Point& fr,const Point& to,
			    const TypeSet<float>& stops,
			    const TypeSet<OD::Color>& colors);
	void		set(const OD::Color&,const OD::Color&,bool hor);
	bool		hasGradient()	{ return !stops_.isEmpty(); }

	Point			from_ = Point::udf();
	Point			to_ = Point::udf();
	TypeSet<float>		stops_;
	TypeSet<OD::Color>	colors_;
    };

    FillGradient		fillgradient_;

    int				zvalue_ = 1; //!<overlay zvalue ( max=on top )
    MouseCursor			cursor_;

    bool			close_ = false;

    void			setFillPattern( const FillPattern& fp )
						{ fillpattern_ = fp; }
    bool			isEmpty() const;
    void			empty();
    bool			turnon_ = true;
    bool			needsupdatelines_ = true;

protected:
				AuxData(const char* nm);
				AuxData(const AuxData&);
				friend class Viewer;
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
				~AxisData();

	BufferString		name_;
	SamplingData<float>	sampling_;
	bool			hasannot_ = true;	// left, bottom
	bool			hasannot2_ = true;	// right, top
	bool			showannot_ = false;	// left, bottom
	bool			showannot2_ = false;	// right, top
	bool			showgridlines_ = false;
	bool			reversed_ = false;
	bool			annotinint_ = false;
	int			factor_ = 1;
	bool			showauxannot_ = true;
	uiString		auxlabel_;
	OD::LineStyle		auxlinestyle_ = OD::LineStyle::Dot;
	OD::LineStyle		auxhllinestyle_;
	TypeSet<PlotAnnotation> auxannot_;
	int			auxPosIdx(float pos,float eps) const;
	void			showAll(bool yn);
    };


				Annotation(bool darkbg);
				~Annotation();

    BufferString		title_; //!< color not settable
    bool			dynamictitle_ = true;
				//!< allows for setting title dynamically
    OD::Color			color_; //!< For axes
    AxisData			x1_;
    AxisData			x2_;

    bool			showaux_ = true;
    bool			showscalebar_ = false;
    bool			showcolorbar_ = false;
    bool			editable_ = false;
    bool			allowuserchange_ = true;
    bool			allowuserchangereversedaxis_ = true;

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
    //bool		haveAux() const;

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
    mExpClass(General) Common
    {
    public:
				Common();
				~Common();

	bool			show_ = true;
	bool			blocky_ = false;
	bool			allowuserchange_ = true;
	bool			allowuserchangedata_ = true;
	ColTab::MapperSetup	mappersetup_;
    };

    //!\brief Variable Density (=color-bar driven) parameters
    mExpClass(General) VD : public Common
    {
    public:

			VD();
			~VD();

	BufferString	ctab_;
	bool		lininterp_ = false; // Use bi-linear interpol, not poly
    };
    //!\brief Wiggle/Variable Area parameters
    mExpClass(General) WVA : public Common
    {
    public:

			WVA();
			~WVA();

	OD::Color	wigg_ = OD::Color::Black();
	OD::Color	refline_ = OD::Color::NoColor();
	OD::Color	lowfill_ = OD::Color::NoColor();
	OD::Color	highfill_ = OD::Color::Black();
	float		overlap_ = 1.f;
	float		reflinevalue_ = mUdf(float);
    };

			DataDispPars();
			~DataDispPars();

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
    static const char*	sKeyLinearInter();
    static const char*	sKeyBlocky();
    static const char*  sKeyAutoScale();
    static const char*	sKeyClipPerc();
    static const char*	sKeyWiggCol();
    static const char*	sKeyRefLineCol();
    static const char*	sKeyLowFillCol();
    static const char*	sKeyHighFillCol();
    static const char*	sKeyOverlap();
    static const char*	sKeySymMidValue();
    static const char*	sKeyRefLineValue();
};


/*!
\brief Flatviewer appearance.
*/

mExpClass(General) Appearance
{
public:
			Appearance(bool drkbg=true);
    virtual		~Appearance();

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    Annotation		annot_ = true;
    DataDispPars	ddpars_;

    //Second set of axes, especially handy in case flat viewer is horizontal
    Annotation		secondsetaxes_ = Annotation(true);
    float		anglewithset1_ = 0.f;

    void		setDarkBG(bool yn);
    bool		darkBG() const		{ return darkbg_; }

    void		setGeoDefaults(bool isvert);

protected:

    bool		darkbg_ = true;	//!< Two styles: dark (=black)
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
  usePack() -> sets one of the available packs for wva or vd display or both
  setPack() -> Combination of addPack and usePack.
  removePack() -> removes this pack from the available packs, if necessary
		  it also clears the wva or vd display to no display.
*/

mExpClass(General) Viewer
{
public:

			Viewer();
    virtual		~Viewer();

    virtual Appearance&	appearance();
    const Appearance&	appearance() const
			{ return const_cast<Viewer*>(this)->appearance(); }

    ZAxisTransform*	getZAxisTransform() const
			{ return datatransform_; }
    bool		setZAxisTransform(ZAxisTransform*);
    bool		hasZAxisTransform() const
			{ return datatransform_; }

    enum		VwrDest { WVA, VD, Both, None };
    static VwrDest	getDest(bool dowva,bool dovd);

    void		setPack(VwrDest,RefMan<FlatDataPack>,
				bool usedefs=true);
			//!< Sets datapack to use on either wva or vd or both
    bool		enableChange(bool yn);
			//!< Returns previous state

    WeakPtr<FlatDataPack> getPack(bool wva,bool checkother=false) const;
			/*!<
			 \param wva if true, wiggle, else variable density
			 \param checkother if true, the datapack of other
			 display (i.e. variable density or wiggles) is returned
			 if the specified display has no datapack. */

    virtual bool	isVertical() const		{ return true; }
    bool		isVisible(bool wva) const;
    bool		isVisible(VwrDest) const;
    bool		setVisible(VwrDest,bool visibility,
				   od_uint32* ctype=nullptr);
			/*!< Will also handleChange unless ctype is provided,
			     So, do not use unless you want both.
			     returns if handled */

    Coord3		getCoord(const Point&) const;

    enum DataChangeType	{ All=0xFFFFFFFF, BitmapData=0x0001, DisplayPars=0x0002,
			  Annot=0x0004, Auxdata=0x0008 };

    virtual void	handleChange(unsigned int datachangetype)	= 0;
			/*!<\param datachangetype can be any combination of
				   DataChangeType. */

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

    virtual AuxData*		createAuxData(const char* nm) const	= 0;

    virtual int			nrAuxData() const			= 0;
    virtual AuxData*		getAuxData(int idx)			= 0;
    virtual const AuxData*	getAuxData(int idx) const		= 0;
    virtual void		addAuxData(AuxData* a)			= 0;
    virtual AuxData*		removeAuxData(AuxData* a)		= 0;
    virtual AuxData*		removeAuxData(int idx)			= 0;
    void			removeAuxDatas(ObjectSet<AuxData>&);
    void			removeAllAuxData();
    virtual void		setAnnotChoice(int selannot)		{}
    virtual int			getAnnotChoice(BufferStringSet&) const
				{ return -1; }
    void			enableStatusBarUpdate()
				{ needstatusbarupd_ = true; }
    void			disableStatusBarUpdate()
				{ needstatusbarupd_ = false; }
    bool			needStatusBarUpdate() const
				{ return needstatusbarupd_; }
    void			setSeisGeomidsToViewer(TypeSet<Pos::GeomID>&);
    const TypeSet<Pos::GeomID>&	getAllSeisGeomids() const;
    void			setZDomain(const ZDomain::Def&);
    const ZDomain::Info&	zDomain() const;
    int				nrDec() const;
    static const char*		sKeyIsZSlice();
    static const char*		sKeyWVAData();
    static const char*		sKeyVDData();
    static const char*		sKeyWVAVal();
    static const char*		sKeyVDVal();
    static const char*		sKeyViewZnrDec();

protected:

    TypeSet< ::DataPackID>	ids_;
    Appearance*			defapp_ = nullptr;
    DataPackMgr&		dpm_;
    ZAxisTransform*		datatransform_ = nullptr;
    FlatView_CB_Rcvr*		cbrcvr_;
    mutable Threads::Lock	lock_;
    bool			needstatusbarupd_ = true;
    ZDomain::Info*		zdinfo_;

    void			addAuxInfo(bool,const Point&,IOPar&) const;
    bool			shouldHandleChange() const
				{ return canhandlechange_; }

private:

    WeakPtr<FlatDataPack>	wvapack_ = nullptr;
    WeakPtr<FlatDataPack>	vdpack_ = nullptr;
    TypeSet<Pos::GeomID>	geom2dids_;
    bool			canhandlechange_ = true;

public:
    mDeprecated("No longer required")
    bool		hasPack( bool wva ) const
			{ return packID(wva)!=DataPack::cNoID(); }
    mDeprecated("No longer required")
    DataPackID		packID(bool wva) const;
    mDeprecated("No longer required")
    const TypeSet< ::DataPackID>&	availablePacks() const	{ return ids_; }
    mDeprecated("No longer required")
    bool		isAvailable( ::DataPackID id ) const
			{ return ids_.isPresent(id); }
    mDeprecated("No longer required")
    void		clearAllPacks();
    mDeprecated("No longer required")
    virtual void	removePack(::DataPackID);
			//!< Releases DataPack after removing from the list.
    mDeprecated("No longer required")
    void		removeUnusedPacks();
    mDeprecated("No longer required")
    void		addPack(::DataPackID);
			/*!< Adds to list and obtains the DataPack, but does not
			 use for WVA or VD. DataPack gets released in the
			 destructor of this class. */
    mDeprecated("No longer required")
    void		usePack(VwrDest,::DataPackID,bool usedefs=true);
    mDeprecated("Use VwrDest enum")
    void		usePack( bool wva, ::DataPackID id, bool usedefs=true)
			{ usePack( wva ? WVA : VD, id, usedefs ); }
    mDeprecated("Use version with ConstRefMan argument")
    void		setPack(VwrDest,::DataPackID,bool usedefs=true);
    mDeprecated("Use VwrDest enum")
    void		setPack( bool wva, ::DataPackID id, bool usedefs=true)
			{ setPack( wva ? WVA : VD, id, usedefs ); }
    mDeprecated("Use VwrDest enum")
    void		setVisible( bool wva, bool visibility )
			{ setVisible( wva ? WVA : VD, visibility ); }

    mDeprecated("Use getPack")
    const FlatDataPack*	obtainPack(bool wva,bool checkother=false) const;

};

} // namespace FlatView
