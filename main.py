import ctypes
import os
from netCDF4 import Dataset
import numpy as np
import pandas as pd
from multiprocessing import Pool, cpu_count

def sied(df, nbins, nrows, nbins_in_row, basebins):
    """
    Performs the Belkin-O'Reilly front detection algorithm on the provided bins
    :param total_bins: total number of bins in the binning scheme
    :param nrows: number of rows in the binning scheme
    :param fill_value: value to fill empty bins with
    :param bins: bin numbers for all data containing bins
    :param data: weighted sum of the data for each bin
    :param weights: weights used to calculate weighted sum for each bin
    :param date: date of the temporal bin
    :param chlor_a: if the data is chlorophyll a concentration, the natural lograithm of the data will be used
    in edge detection default is false
    :return: pandas dataframe containing bin values of each bin resulting from edge detection algorithm
    """
    _cayula = ctypes.CDLL('./sied.so')
    data = (ctypes.c_int * nbins)(*df["Data"].tolist())
    print(data[0])
    out_data = (ctypes.c_int * nbins)()
    _cayula.cayula.argtypes = (ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int), ctypes.c_int,
                               ctypes.c_int, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int))
    _cayula.cayula(data, out_data, nbins, nrows, nbins_in_row, basebins)
    df["Data"] = out_data
    return df


def renumber_bins(lats, lons, rows, bins, data, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180):
    """
    Takes coordinates for the original bins and crops to provided extent. Each bin is assigned a new number so that
    the subset of bins begins with 1.
    :args
        :param lats: obj:list: The latitude of each bin. Latitude values should be the same for each bin in a row
        :param lons: obj:list: The longitude of each bin.
        :param rows: obj:list: The row number for each bin.
        :param bins: obj:list: The original bin number for each row
        :param min_lat: float, optional: The minimum latitude. Defaults to -90.
        :param max_lat: float, optional: The maximum latitude. Defaults to 90.
        :param min_lon: float, optional: The minimum longitude. If the desired window crosses the antimeridian,
            the min_lon should be the positive longitude that will bound the left side of the new map. Defaults to -180.
        :param max_lon: float, optional: The maximum longitude. Defaults to 180.
    :returns
        dataframe: Dataframe containing the original bin information as well as the new bin numbers

    """
    lons = np.array(lons)
    """
    if min_lon > 0 and max_lon < 0:
        min_lon -= 360
        lons[lons > 0] -= 360
        """
    df = pd.DataFrame(data={"Latitude": list(lats), "Longitude": lons, "Data":data,"Row": list(rows), "Bin": list(bins)})
    df = df[(df["Latitude"] >= min_lat) & (df["Latitude"] <= max_lat) & (df["Longitude"] >= min_lon) & (
                df["Longitude"] <= max_lon)].sort_values("Bin")
    df = df.sort_values("Bin")
    df["New_Bin"] = np.arange(len(df))
    return df

class DATA(ctypes.Structure):
    _fields_ = [("values", ctypes.POINTER(ctypes.c_double)),("weights", ctypes.POINTER(ctypes.c_double))]

def crop(values, weights, data_bins, total_bins, nrows, chlora):
    """
    Calculates latitude and longitude
    :param total_bins:
    :param nrows:
    :return:
    """
    _cayula = ctypes.CDLL('./sied.so')
    _cayula.define.argtypes = (ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_double),
                                   ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int), ctypes.c_int,
                                   ctypes.c_int)

    _cayula.initialize.argtypes = (DATA, ctypes.POINTER(ctypes.c_int), ctypes.c_int, ctypes.c_int, ctypes.c_int,
                                   ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int), ctypes.c_bool)
    lats = (ctypes.c_double * total_bins)()
    lons = (ctypes.c_double * total_bins)()
    bins = (ctypes.c_int * total_bins)()
    out_rows = (ctypes.c_int * total_bins)()
    _cayula.define(lats, lons, out_rows, bins, nrows, total_bins)

    in_data = DATA((ctypes.c_double * len(values))(*values),(ctypes.c_double * len(weights))(*weights))
    out_data = (ctypes.c_int * len(bins))()
    bins = (ctypes.c_int * len(bins))(*bins)
    data_bins = (ctypes.c_int * len(data_bins))(*data_bins)
    _cayula.initialize(in_data, out_data, len(bins), len(values), total_bins, data_bins, bins, chlora)
    df = renumber_bins(lats, lons, out_rows, bins, out_data, min_lat=10, max_lat=70, min_lon=-180, max_lon=-110)
    nrows = len(df.groupby("Row").count()["Latitude"])
    nbins_in_row = (ctypes.c_int * nrows)(*df.groupby("Row").count()["Latitude"].tolist())
    basebins = (ctypes.c_int * nrows)(*df.drop_duplicates("Row")["New_Bin"].tolist())
    return df, nrows, nbins_in_row, basebins


def get_params_modis(dataset, data_str):
    """
    Parses values from netCDF4 file for use in Belkin-O'Reilly algorithm
    :param dataset: netCDF4 object containing data
    :param data_str: string key for data in netCDF4 Dataset object
    :return: the total number of bins in binning scheme, the number of rows, list of all data containing bins,
    data value for each bin, weight value for each bin
    """
    total_bins = np.array(dataset.groups["level-3_binned_data"]["BinIndex"][:].tolist())[:, 3].sum()
    nrows = len(dataset.groups["level-3_binned_data"]["BinIndex"])
    binlist = np.array(dataset["level-3_binned_data"]["BinList"][:].tolist())
    bins = binlist[:, 0].astype("int")
    weights = binlist[:, 3]
    data = np.array(dataset.groups["level-3_binned_data"][data_str][:].tolist())[:, 0]
    date = dataset.time_coverage_start

    return total_bins, nrows, bins, data, weights, date


def map_bins(dataset, latmin, latmax, lonmin, lonmax, glob):
    """
    Takes a netCDF4 dataset of binned satellite data and creates a geodataframe with coordinates and bin data values
    :param dataset: netCDF4 dataset containing bins and data values
    :param latmin: minimum latitude to include in output
    :param latmax: maximum latitude to include in output
    :param lonmin: minimum longitude to include in output
    :param lonmax: maximum longitude to include in output
    :return: geodataframe containing latitudes, longitudes, and data values of all bins within given extent
    """
    if glob:
        total_bins, nrows, rows, bins, data, weights, date = get_params_glob(dataset, "chlor_a")
    else:
        total_bins, nrows, bins, data, weights, date = get_params_modis(dataset, "chlor_a")
        rows = []
    df, nrows, nbins_in_row, basebins = crop(data, weights, bins, total_bins, nrows, True)
    df = sied(df, df.shape[0], nrows, nbins_in_row, basebins)
    df = df[df['Data'] > -999]
    return df


def map_file(args):
    cwd = os.getcwd()
    dataset = Dataset(args["file"])
    year_month = dataset.time_coverage_start[:7]
    date = dataset.time_coverage_start[:10]
    if args["file"].endswith("SNPP_CHL.nc"):
        outfile = date + "viirs_chlor.csv"
    else:
        outfile = date + '_chlor.csv'

    if outfile not in args["outfiles"]:
        df = map_bins(dataset, args["latmin"], args["latmax"], args["lonmin"], args["lonmax"], args["glob"])
        dataset.close()
        if not os.path.exists(cwd + "/out/" + year_month):
            os.makedirs(cwd + "/out/" + year_month)
        try:
            df.to_csv(cwd + "/out/" + year_month + "/" + outfile, index=False)
        except IOError as err:
            print("Error while attempting to save shapefile:", err)

        print("Finished writing file %s", outfile)


def map_files(directory, latmin, latmax, lonmin, lonmax):
    """
    Takes a directory of netCDF4 files of binned satellite data and creates shapefiles containing the values from
    the edge detection algorithm for each bin
    :param directory: directory path containing all netCDF4 files
    :param latmin: minimum latitude to include in output
    :param latmax: maximum latitude to include in output
    :param lonmin: minimum longitude to include in output
    :param lonmax: maximum longitude to include in output
    """
    cwd =  "../" + os.getcwd()
    glob = False
    files = []
    outfiles = []
    if not os.path.exists(cwd + "/out"):
        os.makedirs(cwd + "/out")
    for root, dirs, file_names in os.walk(cwd + "/out"):
        for file in file_names:
            if file.endswith(".csv"):
                outfiles.append(file)
    outfiles.sort()
    for file in os.listdir(directory):
        if file.endswith(".nc"):
            files.append({"file": directory + "/" + file, "latmin": latmin,
                          "latmax": latmax, "lonmin": lonmin, "lonmax": lonmax, "glob": glob, "outfiles": outfiles})

    pool = Pool(1)
    pool.map(map_file, files)


def main():
    cwd = os.getcwd()
    map_files(cwd + "/input", -60, 60, -180, 0)


if __name__ == "__main__":
    main()
