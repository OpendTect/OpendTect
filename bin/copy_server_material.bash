#!/bin/bash

# ---- Make these right

binsubdir=lux64
relsubdir=Debug

qtdir=/apps/qt_inst
#qtdir=/dsk/d42/bert/dev/qt_inst

ourlibs="Geometry General Basic Seis Well ODHDF5 Strat Algo MMProc Network"
qtlibs="Qt5Core Qt5Network"
#qtextralibs="libicudata libicui18n libicuuc"

# ---- Should be OK from here on

if [ -z $2 ]; then
    echo "Usage: $0 input_od7_directory output_server_dir_to_zip"
    exit 1
fi


inp=$1
out=$2

if [ ! -d $inp ]; then
    echo "Input directory $inp not found"
    exit 1
fi

if [ -d $out ]; then
    echo "$out already exists"
    exit 1
fi

mkdir $out $out/bin $out/data
mkdir $out/bin/$binsubdir

outexecsdir=$out/bin/$binsubdir/$relsubdir
outrelinfodir=$out/relinfo
inpexecsdir=$inp/bin/$binsubdir/$relsubdir
mkdir $outexecsdir $outrelinfodir

echo -n "Copying libs ... "
for lib in $ourlibs
do
    cp $inpexecsdir/lib$lib.so $outexecsdir
done
echo "done."

qtlibdir=$qtdir/lib
echo -n "Copying Qt libs ... "
for lib in $qtlibs
do
    cp $qtlibdir/lib$lib.so.5 $outexecsdir
done
for lib in $qtextralibs
do
    cp $qtlibdir/lib$lib.so.56 $outexecsdir
done
echo "done."

echo -n "Copying data ... "
cp -a $inp/data/ModDeps.od $out/data
echo "done."

echo -n "Copying core material ... "
cp $inpexecsdir/od_*Man $outexecsdir
echo "7.0.0" > $outrelinfodir/ver_base_$binsubdir.txt
echo "7.0.0" > $outrelinfodir/ver.basedata.txt
echo "done."
