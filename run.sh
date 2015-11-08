trace="trace2.txt"
mod="250"
fsize="14336"
numtrials="2"
idlist="id_list.txt"
db="db.txt"
dbdir="db-files"
linelen="5"
numlines="5"

./db-gen $trace $mod $fsize $idlist $db $dbdir

./benchmark 0 0 $numtrials $idlist $fsize $dbdir $mod
./benchmark 0 1 $numtrials $idlist $fsize $dbdir $mod
./benchmark 0 2 $numtrials $idlist $fsize $dbdir $mod $trace

./benchmark 1 0 $numtrials $idlist $fsize $db 0
./benchmark 1 1 $numtrials $idlist $fsize $db 0
./benchmark 1 2 $numtrials $idlist $fsize $db $trace 0

./benchmark 1 0 $numtrials $idlist $fsize $db 1 $linelen $numlines
./benchmark 1 1 $numtrials $idlist $fsize $db 1 $linelen $numlines
./benchmark 1 2 $numtrials $idlist $fsize $db $trace 1 $linelen $numlines

rm -rf $dbdir $db $idlist
