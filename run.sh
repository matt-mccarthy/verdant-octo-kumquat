./db-gen trace2.txt 250 14336 id_list.txt db.txt db-files

echo $(./benchmark 0 0 2 id_list.txt 14336 db-files 250)
echo $(./benchmark 0 1 2 id_list.txt 14336 db-files 250)
echo $(./benchmark 0 2 2 id_list.txt 14336 db-files 250 trace2.txt)

echo $(./benchmark 1 0 2 id_list.txt 14336 db.txt 0)
echo $(./benchmark 1 1 2 id_list.txt 14336 db.txt 0)
echo $(./benchmark 1 2 2 id_list.txt 14336 db.txt trace2.txt 0)

echo $(./benchmark 1 0 2 id_list.txt 14336 db.txt 1 5 5)
echo $(./benchmark 1 1 2 id_list.txt 14336 db.txt 1 5 5)
echo $(./benchmark 1 2 2 id_list.txt 14336 db.txt trace2.txt 1 5 5)

rm -rf db-files db.txt id_list.txt
