#!/bin/bash
fecha=`date +%d-%m-%Y`
archivo="dumps/collabOffice-$fecha.sql"
mysqldump --user=root --password=slack142 --host=localhost --routines --triggers collabOffice > $archivo
chmod 777 $archivo



