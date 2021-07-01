if (!exists("inputfile")) {
  inputfile='default.csv'
}
if (!exists("outputfile")) {
  outputfile='default.eps'
}
if (!exists("metric")) {
  metric='unknown'
}

set terminal postscript eps enhanced color "Times-Roman"
set encoding utf8
set output outputfile
set nokey 

set size 1.2,0.4
set xlabel "Time (hr)" offset 0,0.5
set grid
set linetype 1 lc rgb "midnight-blue" lw 2 pt 6 ps 1

if(metric eq 'speed') {
  # set title "Time vs Speed" offset 0,-0.8
  set ylabel "Speed (1000 km/hr)" offset 1,-0.5
}
if(metric eq 'area') {
  set yrange[0:2.1]
  set ytics 0,1,2.1
  # set title "Time vs Residential Areas (By TopK Method)" offset 0,-0.8
  set ylabel "Frequent Locations (ID)" offset 1,-0.5
}

set xdata time
set timefmt '%H:%M:%S'
set format x "%H"

set datafile separator ","
set style data points

if(metric eq 'speed') {
  plot inputfile using 1:($2)/1000 lt 1
}
else {
  plot inputfile using 1:2 lt 1
}
