#! /bin/sh

scripts/run-fast-tests.sh

. scripts/testenv.sh

cd $GOLDEN_DIR
for i in *.txt; do 
	dos2unix -q $i
done
