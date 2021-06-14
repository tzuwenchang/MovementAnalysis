for method in average gravity mindist
do
  for areaID in 1 2
  do
    gnuplot -e "inputfile='${method}-area-${areaID}.csv'; outputfile='${method}-area-${areaID}.eps'; method='${method}'; areaID='${areaID}'" linespoints_cdf.plt
  done
done

for metric in speed area
do
  gnuplot -e "inputfile='time-vs-${metric}.csv'; outputfile='time-vs-${metric}.eps'; metric='${metric}'" points_by_time.plt
done