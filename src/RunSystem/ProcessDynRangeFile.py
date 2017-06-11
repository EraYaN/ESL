import os
import sys
import argparse as ap
import csv
import numpy as np
import pandas as pd
import pprint
import matplotlib.pyplot as plt
plt.style.use('ggplot')

class DynRangeProcessor(object):
    NUM_BINS = 50
    FUNCTION_HEADER = 'func'
    VALUE_HEADER = 'v'

    def __init__(self, file):
        if(os.path.exists(file)):
            self.filename = file
        else:
            raise FileNotFoundError("File {} does not exist".format(file))

    def ReadData(self):
        df = pd.read_csv(self.filename,header=0)
        df_E = {}
        df_p = {}
        df_C = {}

        print("Sorting...")
        df.sort_values(self.FUNCTION_HEADER, inplace=True)   
        print("Creating index...")
        df.set_index(keys=[self.FUNCTION_HEADER], drop=False,inplace=True)             
        print("Getting list of all unique values in index...")
        funcs=df[self.FUNCTION_HEADER].unique().tolist()
        print("Splitting off Epanechnikov_kernel...")
        df_E = df.loc[df[self.FUNCTION_HEADER]=='E']
        print("Splitting off pdf_representation...")
        df_p = df.loc[df[self.FUNCTION_HEADER]=='p']
        print("Splitting off CalWeight...")
        df_C = df.loc[df[self.FUNCTION_HEADER]=='C']
        
        print("Starting main processing...")

        print("Epanechnikov_kernel data:")
        self.Process(df_E,"E")
        print("pdf_representation data:")
        self.Process(df_p,"p")
        print("CalWeight data:")
        self.Process(df_C,"C")
        

    def Process(self,df,name):
        print("Replacing all inf and -inf values with NaN...")
        #df=df.replace([np.inf], 20000)
        df=df.replace([np.inf, -np.inf], np.nan)
        #df=df.replace([-np.inf], -20000)
        print("Dropping all nan values...")
        count = len(df)
        df=df.dropna(subset=[self.VALUE_HEADER], how="all")
        count_after = len(df)
        print("Size adjusted from {} to {}, so {} NaN values were dropped.".format(count,count_after,count-count_after))
        print("Calculating extremes...")
        min = df[self.VALUE_HEADER].min()
        max = df[self.VALUE_HEADER].max()
        print("Min: {}".format(min))
        print("Max: {}".format(max))
        print("Binning...")

        bin_margin_size = ((max-min)/self.NUM_BINS)/self.NUM_BINS

        top_bin = max + bin_margin_size
        bottom_bin = min - bin_margin_size

        bins = np.linspace(bottom_bin, top_bin, self.NUM_BINS)
        tmp = np.digitize(df[self.VALUE_HEADER], bins)
        groups = df.groupby(tmp,sort=False)
        
        bin_counts = groups[self.VALUE_HEADER].count()
        
        bin_values = bin_counts.values
        bin_keys = list(bin_counts.keys())

        axislabels = []
        axisvalues = []
        for i in range(1,self.NUM_BINS):
            axislabels.append("[{:.1f}, {:.1f})".format(bins[i-1],bins[i]))
            if i in bin_keys:
                axisvalues.append(bin_values[bin_keys.index(i)])
            else:
                axisvalues.append(0)
        print("Bins:")
        pprint.pprint(list(zip(axislabels, axisvalues)))
        #groups = df.groupby(['username', pd.cut(df.views, bins)])

        #mean = groups.mean()
        #print("Mean: {}".format(mean)) 

        #median = groups.median()
        #print("Median: {}".format(median))

        print("Plotting histogram (bar)...")         
        #counts.plot.barh()
        plt.figure()        
        numbers = range(len(axisvalues))
        plt.bar(numbers, axisvalues)
        ax = plt.gca()
        ax.set_yscale('log')
        #ax.set_xticklabels(ax.xaxis.get_majorticklabels(), rotation=45)
        ax.set_xticklabels(axislabels, rotation=45)
        plt.grid(b='on')
        plt.tight_layout()
        plt.savefig('histogram{}.pdf'.format(name), format='pdf')        
        plt.close()
        print("Plotting done.")

if __name__ == '__main__':
    parser = ap.ArgumentParser(prog='ESLab RunSystem DynRange Processor',description='ESLab RunSystem DynRange Processor')
    parser.add_argument('--file', action="store", help='The dynrange.csv file',default='dynrange.csv')    
    print("Starting...")
    try:
        opts = parser.parse_args(sys.argv[1:])
        
        drp = DynRangeProcessor(opts.file)
        print("Reading in data...")
        drp.ReadData() 

       

        print("Done.")
    except SystemExit:
        print('Bad Arguments')