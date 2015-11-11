# Run: sh run.sh trace.txt 250 14336 5 75 200
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


./db-gen $trace $mod $fsize $idlist $db $dbdir

#echo "Done generating"

./benchmark $mode_dir $read_cont $numtrials $idlist $fsize $dbdir $mod
./benchmark $mode_dir $read_rand $numtrials $idlist $fsize $dbdir $mod
./benchmark $mode_dir $read_trace $numtrials $idlist $fsize $dbdir $mod $trace

./benchmark $mode_db $read_cont $numtrials $idlist $fsize $db $db_raw
./benchmark $mode_db $read_rand $numtrials $idlist $fsize $db $db_raw
./benchmark $mode_db $read_trace $numtrials $idlist $fsize $db $trace $db_raw

./benchmark $mode_db $read_cont $numtrials $idlist $fsize $db $db_ram
./benchmark $mode_db $read_rand $numtrials $idlist $fsize $db $db_ram
./benchmark $mode_db $read_trace $numtrials $idlist $fsize $db $trace $db_ram

./benchmark $mode_db $read_cont $numtrials $idlist $fsize $db $db_cache $linelen $numlines
./benchmark $mode_db $read_rand $numtrials $idlist $fsize $db $db_cache $linelen $numlines
./benchmark $mode_db $read_trace $numtrials $idlist $fsize $db $trace $db_cache $linelen $numlines

rm -rf $dbdir $db $idlist
