if (!exists("inputfile")) {
  inputfile='default.csv'
}
if (!exists("outputfile")) {
  outputfile='default.eps'
}
if (!exists("method")) {
  method='unknown'
}
if (!exists("areaID")) {
  areaID='unknown'
}

set terminal postscript eps enhanced color "Times-Roman" 
set encoding utf8
set output outputfile
set nokey 

set size 0.45,0.6
set xlabel "Center Distance (km)" offset 0,0.25
set yrange[0:100]
set ytics 0,10,100
set ylabel "Cumulative Percentage of Logs (%)" offset 2,-0.5

set grid
set linetype 1 lc rgb "midnight-blue" lw 2 pt 6 ps 1

if(method eq 'average') {
  set title "Average Latitude/Longitude" offset 0,-0.8
}
if(method eq 'gravity') {
  set title "Center of Gravity" offset 0,-0.8
}
if(method eq 'mindist') {
  set title "Center of Minimum Distance" offset 0,-0.8
}

set datafile separator ","
set style data linespoints

plot inputfile using 1:2 lt 1

