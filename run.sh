./db-gen trace2.txt 250 14336 id_list.txt db.txt db-files

./benchmark 0 0 2 id_list.txt 14336 db-files 250
./benchmark 0 1 2 id_list.txt 14336 db-files 250
./benchmark 0 2 2 id_list.txt 14336 db-files 250 trace2.txt

./benchmark 1 0 2 id_list.txt 14336 db.txt
./benchmark 1 1 2 id_list.txt 14336 db.txt
./benchmark 1 2 2 id_list.txt 14336 db.txt trace2.txt
