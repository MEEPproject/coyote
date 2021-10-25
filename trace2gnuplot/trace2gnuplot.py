#!/usr/bin/python

import argparse
import json
import numpy as np


def print_gnuplot_header(file):
	file.write('reset session')
	file.write('$data <<EOD')

def print_gnuplot_footer(file, col_count):
	file.write('EOD')
	file.write('set xlabel "time"')
	file.write('set xrange [:]')
	file.write('set yrange [:] reverse')
	file.write('set offsets 0,0,0.5,0.5')
	file.write('set style fill solid 1.0')
	file.write('set key out')
	file.write('ColCount = '+str(col_count))
	file.write('myBoxwidth = 0.8')
	file.write('plot for [col=2:ColCount+1] $data u col:0: \\')
	file.write('\t(total=(sum [i=2:ColCount+1] column(i)),(sum [i=2:col-1] column(i))): \\')
	file.write('\t((sum [i=2:col] column(i))): \\')
	file.write('\t($0-myBoxwidth/2.):($0+myBoxwidth/2.):ytic(1) w boxxyerror ti columnhead(col)')
	file.write('pause -1 "Hit key to continue"');
	
def print_gnuplot_header_boxes(file):
	file.write('reset session\n')
	file.write('set terminal wxt size 1900,1000\n')
	file.write('set grid\n')
	#file.write('set size ratio -1\n')
	#file.write('set xtics 1\n')
	#file.write('set ytics 1\n')
	#file.write('set format y "%x"\n')
	file.write('set key off\n')
	file.write('set angle degrees\n')
	file.write('set style fill transparent solid 0.5\n')

def print_gnuplot_footer_boxes(file, max, ymin, ymax):
	ymin = str(ymin)
	ymax = str(ymax)
	file.write('set xrange[0:'+str(max)+']\n')
	file.write('set yrange['+str(ymin)+':'+str(ymax)+'.2]\n')
	file.write('plot 0, '+str(ymax)+'\n')
	file.write('pause -1 "Hit key to continue"\n');
	
def set_object(gnufile, labelID, lowerLeft, upperRight, bg, pattern):
	lowerLeft[0] = str(lowerLeft[0])	#-- GNUPLot can only handle 32bit integers
	lowerLeft[1] = str(lowerLeft[1])
	upperRight[0] = str(upperRight[0])
	upperRight[1] = str(upperRight[1])
	string  = 'set obj '+str(labelID)
	string += ' rect from '+(','.join(lowerLeft))+' to '+(','.join(upperRight))
	string += ' fillcolor "'+bg+'"'
	string += ' fill pattern '+str(pattern)+'\n'
	gnufile.write(string);
	print(string)
	
	
def set_label(gnufile, labelID, coord, text):
	coord[0] = str(coord[0])
	coord[1] = str(coord[1])
	string  = 'set label '+str(labelID)
	string += ' at '+(','.join(coord))
	string += ' "'+text.replace('_', ' ')+'" rotate by 90 front center\n'
	gnufile.write(string)
	print(string)

def long2int(long):
	return long & 0x7fffffff #-- MSB is signed bit
	
def main(args):
	
	prevData = {}
	maxValue = 1
	count = 1
	index = 0
	
	
	with open('trace2gnuplot.json', 'r') as jsonfile:
		data = jsonfile.read()
	config = json.loads(data)
	jsonfile.close()
	
	gnufile = open(args.output, 'w')
	print_gnuplot_header_boxes(gnufile)
	
	with open(args.trace, 'r') as tracefile:
		
		
		for line in tracefile:
			data = line.strip().split(',');
			# data[0]: time index
			# data[1]: ?
			# data[2]: address/address of parent instruction
			# data[3]: event
			# data[4]: ?
			# data[5]: address
			
			print(line+'\n')
			
			#-- search for the address in the address array
			try:
				index = args.address.index(data[2]);
				#print(data[2]+' '+str(index)+' ')
				if(config['config'][data[3]]['has_parent_op'] or config['config'][data[3]]['address_in_pos5']):
					try:
						index = args.address.index(data[5])
					except ValueError as e:
						index = len(args.address)
						args.address.append(data[5])
						#print('appended, index now='+str(index)+' ')
						#print(args.address)
					data[2] = data[5]
					
			except ValueError as e:
				try:
					index = args.address.index(data[5]);
					data[2] = data[5]
				except ValueError as e:
					continue
				

			
			print(str(args.address)+' '+str(index)+' '+str(line));
			
			try:
				test = prevData[index]
			except KeyError as e:
				prevData[index] = data
				continue;
			
			set_object(gnufile, count, [int(prevData[index][0]), index], [int(data[0]), index+1], config['config'][prevData[index][3]]['bg'], config['config'][prevData[index][3]]['pattern'])		
			set_label(gnufile, count, [int(prevData[index][0])+(int(data[0])-int(prevData[index][0]))/2.0, (index+index+1)/2.0], prevData[index][3]+' ('+ prevData[index][2]+')')
			prevData[index] = data
			maxValue = data[0]
			count = count + 1

	#-- last entries
	for i in prevData:
		set_object(gnufile, count, [int(prevData[i][0]), i], [int(prevData[i][0]), i+1], config['config'][prevData[i][3]]['bg'], config['config'][prevData[i][3]]['pattern'])
		set_label(gnufile, count, [int(prevData[i][0])+(int(prevData[i][0])-int(prevData[i][0]))/2.0, (2*i+1)/2.0], prevData[i][3]+' ('+prevData[i][2]+')')
		count = count + 1
			
			
	key_list = [key for key in prevData]
	ymin = np.min(key_list);
	ymax = np.max(key_list);
	print_gnuplot_footer_boxes(gnufile, maxValue, ymin, ymax)
	gnufile.close()
	

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('-t', '--trace', type=str, help='trace file', required=True);
	parser.add_argument('-o', '--output', type=str, help='gnuplot file', required=True);
	parser.add_argument('-a', '--address', type=str, nargs='+', help='address(es) to be traced', required=True);
	args = parser.parse_args()	
	main(args)