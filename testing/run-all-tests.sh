#! /bin/sh

scripts/run-all-tests.sh

. scripts/testenv.sh

cd $GOLDEN_DIR
for i in *.txt; do 
	dos2unix -q $i
done
