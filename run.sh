# Run: sh run.sh trace.txt 250 14336 5 1 15000
trace=$1
mod=$2
fsize=$3
numtrials=$4
linelen=$5
numlines=$6
idlist="id_list.txt"
db="./ext/db.txt"
dbdir="./ext/db-files"
mode_dir="0"
mode_db="1"
read_cont="0"
read_rand="1"
read_trace="2"
db_raw="0"
db_ram="1"
db_cache="2"

echo "Generating Test Environment"

./db-gen $trace $mod $fsize $idlist $db $dbdir

echo "Done generating"
echo "Directory"
echo "Continuous"
#./benchmark $mode_dir $read_cont $numtrials $idlist $fsize $dbdir $mod
echo "Random"
#./benchmark $mode_dir $read_rand $numtrials $idlist $fsize $dbdir $mod
echo "Trace"
#./benchmark $mode_dir $read_trace $numtrials $idlist $fsize $dbdir $mod $trace
echo "DB Raw"
echo "Continuous"
#./benchmark $mode_db $read_cont $numtrials $idlist $fsize $db $db_raw
echo "Random"
#./benchmark $mode_db $read_rand $numtrials $idlist $fsize $db $db_raw
echo "Trace"
#./benchmark $mode_db $read_trace $numtrials $idlist $fsize $db $trace $db_raw
echo "DB RAM"
echo "Continuous"
#./benchmark $mode_db $read_cont $numtrials $idlist $fsize $db $db_ram
echo "Random"
#./benchmark $mode_db $read_rand $numtrials $idlist $fsize $db $db_ram
echo "Trace"
#./benchmark $mode_db $read_trace $numtrials $idlist $fsize $db $trace $db_ram
echo "DB Cache"
echo "Continuous"
#./benchmark $mode_db $read_cont $numtrials $idlist $fsize $db $db_cache $linelen $numlines
echo "Random"
#./benchmark $mode_db $read_rand $numtrials $idlist $fsize $db $db_cache $linelen $numlines
echo "Trace"
./benchmark $mode_db $read_trace $numtrials $idlist $fsize $db $trace $db_cache $linelen $numlines

rm -rf $dbdir $db $idlist
