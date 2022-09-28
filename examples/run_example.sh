# validity checks
if [[ "$(dirname "$0")" != "." ]]; then
  echo 'Please change to the folder containing this script before execution. Exiting...'
  exit 1
fi

TARGET=$1
if [ ! -f $TARGET ]; then
  echo "Invalid example script file name. Exiting..."
  exit 1
fi

# parsing input script path into example folder to be copied and the script file name.
IFS='/'; arrIN=($TARGET); unset IFS;
EXAMPLE_DIR=${arrIN[0]}
RUN_FILE=${arrIN[1]}

# copy latest version of the example to the scratch folder, assuming it lies 3 levels above the examples folder.
SCRATCH_DIR=../../../scratch
TARGET_DIR=$SCRATCH_DIR/$EXAMPLE_DIR
if [[ -d $TARGET_DIR ]]
then
  echo "deleting existing example in scratch folder"
  rm -rf $TARGET_DIR
fi
echo "copying example to scratch folder"
cp -r $EXAMPLE_DIR $SCRATCH_DIR

# cd there and run the script
pushd $TARGET_DIR > /dev/null
python $RUN_FILE
popd > /dev/null
