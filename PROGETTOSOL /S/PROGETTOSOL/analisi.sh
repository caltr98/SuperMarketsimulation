#!/bin/bash
if  !(ls|grep report.log -q);then
	echo "ERRORE analisi.sh:Non esiste file di report" 1>&2
	exit 1
fi
exec 3<report.log
i=0
k=0	
function letturaclienti(){
	local j
	awk 'BEGIN {print"| id cliente | n. prodotti acquistati | tempo totale nel super. | tempo tot. speso in coda | n. di code visitate |"}'
	awk	'BEGIN{ printf "|%5s| |%7s| |%8s| |%8s| |%5s|\n","id", "n.acqu","temptot","codatemp","casse" }' 
	for ((j=0; j<$1; j++)) ; do #clientitotali come limite
		read -u 3 line
		IFS="-" read -r -a arr <<<$line
		local idcliente=${arr[0]#*:}
		local tempoperm=${arr[1]#*:}
		local tempocoda=${arr[2]#*:}
		local cassevisitate=${arr[3]#*:}
		local acquisti=${arr[4]#*:}
		echo $idcliente $acquisti $tempoperm $tempocoda $cassevisitate\
		|awk '{ printf "|%5s| |%7s| |%8.3f| |%8.3f| |%5s|\n", $1, $2,$3,$4,$5,$6 }' 
	done
}
function letturacasse(){
awk 'BEGIN {print"| id cassa | n. prodotti elaborati | n. di clienti | tempo tot. di apertura | tempo medio di servizio | n. di chiusure |"}'
awk	'BEGIN{ printf "|%5s| |%7s| |%6s| |%8s| |%8s| |%5s|\n","id", "n.prod","n.cli","mediaSer","TotOpen","n.chi" }' 
while read -u 3 linea; do
	#echo $linea	
	IFS="-" read -r -a arr <<<$linea 
	local idcassa=${arr[0]#*:}
	local numclienti=${arr[1]#*:}
	local chiusure=${arr[2]#*:}
	local prodotti=${arr[3]#*:}
	#echo $idcassa
	read -u 3 line
	IFS="-" read -r -a array  <<<$line
	local conta=${#array[@]}	
	IFS='+' sum=$(echo "scale=6;${array[*]}"|bc)
	if [ $conta -gt 0 ]; then
		local media=$(echo "scale=6;$sum/$conta"|bc)
	else media=0 
	fi
	#echo conta cosa $conta media $media
	read -u 3 line
	read -u 3 line
	IFS="-" read -r -a array  <<<$line
	if [ ${#array[@]} -eq 0 ];then
		local tempoapertura=0
	else
		IFS='+' tempoapertura=$(echo "scale=6;${array[*]}"|bc)
	fi
	#echo  media=$media
	echo $idcassa $prodotti $numclienti $media $tempoapertura $chiusure \
	|awk '{ printf "|%5s| |%7s| |%6s| |%8.3f| |%8.3f| |%5s|\n", $1, $2,$3,$4,$5,$6 }' 
done



}
read -u 3 line
read -u 3 line
temp=${line#*:}
temp=${temp%-*}
clientitotali=$temp
echo $clientitotali
temp=${line#*-}
temp=${temp#*:}
totaleprodotti=$temp
echo $totaleprodotti
read -u 3 line
read -u 3 line
#chiamo funzione
letturaclienti clientitotali
read -u 3 line
read -u 3 line
read -u 3 line
#chiamo funzione
letturacasse


