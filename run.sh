# Run: sh run.sh trace.txt 250 14336 5 75 200
trace=$1
mod=$2
fsize=$3
numtrials=$4
linelen=$5
numlines=$6
idlist="id_list.txt"
db="db.txt"
dbdir="db-files"

./db-gen $trace $mod $fsize $idlist $db $dbdir

./benchmark 0 0 $numtrials $idlist $fsize $dbdir $mod
./benchmark 0 1 $numtrials $idlist $fsize $dbdir $mod
./benchmark 0 2 $numtrials $idlist $fsize $dbdir $mod $trace

./benchmark 1 0 $numtrials $idlist $fsize $db 0
./benchmark 1 1 $numtrials $idlist $fsize $db 0
./benchmark 1 2 $numtrials $idlist $fsize $db $trace 0

./benchmark 1 0 $numtrials $idlist $fsize $db 1
./benchmark 1 1 $numtrials $idlist $fsize $db 1
./benchmark 1 2 $numtrials $idlist $fsize $db $trace 1

./benchmark 1 0 $numtrials $idlist $fsize $db 2 $linelen $numlines
./benchmark 1 1 $numtrials $idlist $fsize $db 2 $linelen $numlines
./benchmark 1 2 $numtrials $idlist $fsize $db $trace 2 $linelen $numlines

rm -rf $dbdir $db $idlist
