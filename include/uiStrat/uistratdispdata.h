#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uistratmod.h"
#include "callback.h"
#include "color.h"
#include "ranges.h"
#include "bufstring.h"
#include "bufstringset.h"


namespace Strat
{
    class UnitRef;
    class LeavedUnitRef;
    class NodeOnlyUnitRef;
    class NodeUnitRef;
    class RefTree;
}

class uiStratRefTree;
class uiTreeViewItem;

mExpClass(uiStrat) StratDispData
{
public:
			StratDispData() {}
			~StratDispData() { eraseData(); }

    mStruct(uiStrat) Unit
    {
			Unit(const char* nm, const char* fullcode=0,
				    const OD::Color& col = OD::Color::White() )
				: name_( nm )
				, fullcode_(fullcode)
				, color_(col)
				, isdisplayed_(true)
				{}

	const char* 	name() const 	{ return name_.buf(); }
	const char* 	fullCode() const { return fullcode_.buf(); }

	OD::Color	color_;
	Interval<float>	zrg_;
	bool		isdisplayed_;

	int 		colidx_; //tree depth

    protected :

	BufferString 	name_;
	BufferString    fullcode_;
    };


    mStruct(uiStrat) Level
    {
			    Level(const char* nm,const char* unitcode)
				    : unitcode_(unitcode)
				    , name_( nm )
				    {}

	const BufferString  name_;
	const BufferString  unitcode_;
	OD::Color	    color_;
	float		    zpos_;
    };


    mStruct(uiStrat) Column
    {
			Column( const char* nm )
			    : name_(nm)
			    , isdisplayed_(true)
			    {}
			~Column()
			{
			    deepErase( units_ );
			    deepErase( levels_ );
			}

	const BufferString name_;
	bool		isdisplayed_;

	ObjectSet<Unit>	units_;
	ObjectSet<Level> levels_;
    };



    void		eraseData()
			{
			    deepErase( cols_ );
			}

    void		addCol( Column* col )
			    { cols_ += col; }

    int 		nrCols() const
			    { return cols_.size(); }
    int			nrUnits( int colidx ) const
			    { return cols_[colidx]->units_.size(); }
    void		addUnit( int colidx, Unit* un )
			    { cols_[colidx]->units_ += un; un->colidx_=colidx; }

    const Column*	getCol( int idx ) const
			    { return cols_[idx]; }
    Column*		getCol( int idx )
			    { return cols_[idx]; }
    Unit*		getUnit( int colidx, int uidx )
			    { return gtUnit( colidx, uidx ); }
    const Unit*		getUnit( int colidx, int uidx ) const
			    { return gtUnit( colidx, uidx ); }

    int			nrLevels( int colidx ) const
			    { return cols_[colidx]->levels_.size(); }
    const Level*	getLevel( int colidx, int lidx ) const
			    { return cols_[colidx]->levels_[lidx]; }

    int 		nrDisplayedCols() const
			{
			    int nr = 0;
			    for ( int idx=0; idx<cols_.size(); idx++)
				{ if ( cols_[idx]->isdisplayed_ ) nr++; }
			    return nr;
			}

protected :

    Unit*		gtUnit( int colidx, int uidx ) const
			    { return const_cast<Unit*>(
					cols_[colidx]->units_[uidx] ); }

    ObjectSet<Column> cols_;
};



/*!\brief used to gather all units and tied levels from a tree for display*/

mExpClass(uiStrat) uiStratTreeToDisp : public CallBacker
{
public:
			uiStratTreeToDisp(StratDispData&,
					bool withaux=true,
					bool withlvls=true);
			~uiStratTreeToDisp();

    void		setTree();
    Notifier<uiStratTreeToDisp> newtreeRead;

    int			levelColIdx() const 	{ return levelcolidx_; }

protected:

    StratDispData& 	data_;
    Strat::RefTree*	tree_;

    bool 		withauxs_; //lithologies & descriptions
    bool 		withlevels_;
    int			lithocolidx_;
    int			desccolidx_;
    int			levelcolidx_;

    void		readUnits();
    void		addUnit(const Strat::NodeUnitRef&);
    void		addDescs(const Strat::LeavedUnitRef&);
    void		addLithologies(const Strat::LeavedUnitRef&);
    void		addLevel(const Strat::LeavedUnitRef&);
    void		addAnnot(const char*,Interval<float>& posrg,bool);
    void 		readFromTree();

    void		triggerDataChange(CallBacker*);
    void		treeDel(CallBacker*);
};


/*!brief used to write directly in the treeView of the uiStratRefTree
   as if we were handling the uiTree directly*/

mExpClass(uiStrat) uiStratDispToTree : public CallBacker
{
public:
			uiStratDispToTree(uiStratRefTree&);
			~uiStratDispToTree();

    uiTreeViewItem*	getItemFromTree(const char*);

    void		handleUnitMenu(const char*);
    void		handleUnitProperties(const char*);
    void		setUnitLvl(const char*);
    void		addUnit(const char*);

protected:

    uiStratRefTree&     uitree_;

public:
    uiTreeViewItem*	setCurrentTreeItem(const char*);
};
